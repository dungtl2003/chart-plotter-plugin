#include "ChartPlotter/data/DataBufferUpdater.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"
#include "ChartPlotter/utils/Variant.hpp"

#include <QDateTime>

#include <charconv>
#include <limits>

namespace ChartPlotter {

namespace {

// --- Date span parsing -------------------------------------------------------
//
// The Date column is the ingest hot spot: the old path built a QString +
// QVariant per cell and let Qt probe formats. We instead pick a parse strategy
// once per column (validated to reproduce the original QVariant result, so
// x-axis values never shift) and reuse it for every row.

inline bool isDigit(char c) { return c >= '0' && c <= '9'; }

inline std::string_view trimSpan(std::string_view s) {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) {
    s.remove_prefix(1);
  }
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) {
    s.remove_suffix(1);
  }
  return s;
}

// Days from 1970-01-01 for a proleptic-Gregorian date (Howard Hinnant's
// algorithm). Lets us reach epoch-ms without constructing a QDate/QDateTime.
inline qint64 daysFromCivil(qint64 y, unsigned m, unsigned d) {
  y -= m <= 2;
  const qint64 era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = (153u * (m + (m > 2 ? -3 : 9)) + 2u) / 5u + d - 1u;
  const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
  return era * 146097 + static_cast<qint64>(doe) - 719468;
}

inline unsigned read2(const char *p) {
  return static_cast<unsigned>((p[0] - '0') * 10 + (p[1] - '0'));
}

// Fast path: "YYYY-MM-DD" with optional "[ T]HH:MM[:SS][.fff]" and optional
// trailing 'Z'. Always interpreted as UTC -> epoch ms. Returns false (so the
// caller falls back) for anything it doesn't recognise, including a non-'Z'
// numeric zone offset (we don't do offset math here).
bool fastDateMs(std::string_view s, double &out) {
  s = trimSpan(s);
  if (s.size() < 10) {
    return false;
  }
  const char *p = s.data();
  if (!(isDigit(p[0]) && isDigit(p[1]) && isDigit(p[2]) && isDigit(p[3]) &&
        p[4] == '-' && isDigit(p[5]) && isDigit(p[6]) && p[7] == '-' &&
        isDigit(p[8]) && isDigit(p[9]))) {
    return false;
  }

  const qint64 year = (p[0] - '0') * 1000 + (p[1] - '0') * 100 +
                      (p[2] - '0') * 10 + (p[3] - '0');
  const unsigned month = read2(p + 5);
  const unsigned day = read2(p + 8);
  if (month < 1 || month > 12 || day < 1 || day > 31) {
    return false;
  }

  unsigned hour = 0, minute = 0, second = 0, milli = 0;
  std::size_t i = 10;
  if (i < s.size() && (s[i] == 'T' || s[i] == ' ')) {
    ++i;
    if (i + 5 > s.size() || !isDigit(s[i]) || !isDigit(s[i + 1]) ||
        s[i + 2] != ':' || !isDigit(s[i + 3]) || !isDigit(s[i + 4])) {
      return false;
    }
    hour = read2(p + i);
    minute = read2(p + i + 3);
    i += 5;
    if (i + 2 < s.size() && s[i] == ':' && isDigit(s[i + 1]) &&
        isDigit(s[i + 2])) {
      second = read2(p + i + 1);
      i += 3;
    }
    if (i < s.size() && s[i] == '.') {
      ++i;
      unsigned scale = 100;
      while (i < s.size() && isDigit(s[i])) {
        if (scale > 0) {
          milli += static_cast<unsigned>(s[i] - '0') * scale;
          scale /= 10;
        }
        ++i;
      }
    }
  }
  if (i < s.size()) {
    if (s[i] == 'Z' && i + 1 == s.size()) {
      // explicit UTC, fine
    } else {
      return false; // trailing offset / junk -> fall back
    }
  }
  if (hour > 23 || minute > 59 || second > 59) {
    return false;
  }

  const qint64 ms = daysFromCivil(year, month, day) * 86400000LL +
                    static_cast<qint64>(hour) * 3600000LL +
                    static_cast<qint64>(minute) * 60000LL +
                    static_cast<qint64>(second) * 1000LL + milli;
  out = static_cast<double>(ms);
  return true;
}

// QDateTime ISO parse (no QVariant boxing / no TextDate probing). Tolerates a
// space date/time separator by normalising it to 'T'.
bool isoDateMs(std::string_view s, double &out) {
  QString text = QString::fromUtf8(s.data(), static_cast<int>(s.size()));
  QDateTime dt = QDateTime::fromString(text, Qt::ISODateWithMs);
  if (!dt.isValid()) {
    dt = QDateTime::fromString(text, Qt::ISODate);
  }
  if (!dt.isValid()) {
    const int sp = text.indexOf(QLatin1Char(' '));
    if (sp > 0) {
      text[sp] = QLatin1Char('T');
      dt = QDateTime::fromString(text, Qt::ISODateWithMs);
      if (!dt.isValid()) {
        dt = QDateTime::fromString(text, Qt::ISODate);
      }
    }
  }
  if (!dt.isValid()) {
    return false;
  }
  out = static_cast<double>(dt.toMSecsSinceEpoch());
  return true;
}

// Correct, reasonably capable parse: the ground truth the faster modes are
// validated against, and the per-cell backstop. Handles ISO (with 'T'), a space
// date/time separator, and finally Qt's generic conversion — a superset of the
// original QVariant path, so results never change for dates that already
// worked.
bool refDateMs(std::string_view s, double &out) {
  QString text = QString::fromUtf8(s.data(), static_cast<int>(s.size()));
  QDateTime dt = QDateTime::fromString(text, Qt::ISODateWithMs);
  if (!dt.isValid()) {
    dt = QDateTime::fromString(text, Qt::ISODate);
  }
  if (!dt.isValid()) {
    const int sp = text.indexOf(QLatin1Char(' '));
    if (sp > 0) {
      QString t2 = text;
      t2[sp] = QLatin1Char('T');
      dt = QDateTime::fromString(t2, Qt::ISODateWithMs);
      if (!dt.isValid()) {
        dt = QDateTime::fromString(t2, Qt::ISODate);
      }
    }
  }
  if (dt.isValid()) {
    out = static_cast<double>(dt.toMSecsSinceEpoch());
    return true;
  }
  // Last resort: Qt's generic string conversion (TextDate, etc.).
  const QVariant value(text);
  return Utils::Variant::variantToDateNumber(value, out);
}

} // namespace

DataBufferUpdater::DataBufferUpdater(std::shared_ptr<DataBuffer> buffer,
                                     QObject *parent)
    : m_buffer(buffer), QObject(parent) {
  m_profTimer.start();
}

void DataBufferUpdater::logProfile() const {
  CP_INFO("[ingest] convert={} ms, appendRow={} ms", m_convertNs / 1'000'000,
          m_appendNs / 1'000'000);
}

void DataBufferUpdater::setConfig(const DataBufferUpdaterConfig &config) {
  m_config = config;

  if (m_config.skipRows < 0) {
    emit errorOccurred(std::move(
        QStringLiteral("DataBufferUpdater::setConfig: skipRows cannot be "
                       "negative, set default to 0")));
    m_config.skipRows = 0;
  }

  m_canInferTypes = true;
  // if (m_config.mode == ChartEnums::DataMode::Static) {
  //   m_canInferTypes = true;
  // }

  if (m_config.totalColumns > 0) {
    for (const auto &schema : m_config.columns) {
      if (schema.idx < 0 || qsizetype(schema.idx) >= m_config.totalColumns) {
        fail(std::move(
            QStringLiteral("DataBufferUpdater::setConfig: column schema index "
                           "%1 exceeds total column count %2")
                .arg(schema.idx)
                .arg(m_config.totalColumns)));
        return;
      }
    }
  }

  m_configLoaded = true;
}

void DataBufferUpdater::reset() {
  if (m_buffer) {
    m_buffer->clear();
  }
  m_currentPhysicalRow = 0;
  // Cached column pointers reference the buffer's column storage; invalidate
  // them so the fast path re-resolves against the rebuilt columns.
  m_colPtrs.clear();
}

void DataBufferUpdater::parseRows(const QVector<DataRow> &rows) {
  if (!m_configLoaded) {
    emit errorOccurred(std::move(
        QStringLiteral("DataBufferUpdater::parseRows: config is not loaded")));
    return;
  }
  if (!m_buffer) {
    emit errorOccurred(std::move(
        QStringLiteral("DataBufferUpdater::parseRows: buffer is not loaded")));
    return;
  }
  if (!m_buffer->isValid()) {
    emit errorOccurred(std::move(QStringLiteral(
        "DataBufferUpdater::parseRows: buffer is in invalid state")));
    return;
  }

  if (rows.size() < 1) {
    return;
  }

  for (auto &row : rows) {
    // CP_DEBUG(row.toString());
    auto result = parseRow(row);
    if (!result) {
      fail(std::move(QString::fromUtf8(result.error().c_str())));
      return;
    }

    m_currentPhysicalRow++;
  }

  emit bufferUpdated();
}

void DataBufferUpdater::parseBatch(const CsvRowBatch &batch) {
  if (!m_configLoaded) {
    emit errorOccurred(std::move(
        QStringLiteral("DataBufferUpdater::parseBatch: config is not loaded")));
    return;
  }
  if (!m_buffer) {
    emit errorOccurred(std::move(
        QStringLiteral("DataBufferUpdater::parseBatch: buffer is not loaded")));
    return;
  }
  if (!m_buffer->isValid()) {
    emit errorOccurred(std::move(QStringLiteral(
        "DataBufferUpdater::parseBatch: buffer is in invalid state")));
    return;
  }

  const qsizetype rowCount = batch.rowCount();

  // Reset per-column staging for this batch (columns already known).
  prepareFastStaging();
  if (m_columnSize > 0) {
    for (auto &col : m_colValues) {
      col.reserve(rowCount);
    }
  }

  qint64 accepted = 0;
  const qint64 convStart = m_profTimer.nsecsElapsed();

  for (qsizetype r = 0; r < rowCount; ++r) {
    const quint32 fBegin = batch.rowStart[r];
    const quint32 fEnd = batch.rowStart[r + 1];
    const qsizetype fieldCount = fEnd - fBegin;

    if (m_currentPhysicalRow < m_config.skipRows) {
      m_currentPhysicalRow++;
      continue;
    }

    if (!m_firstRowParsed) {
      m_firstRowParsed = true;

      // header is light, so we can use QVariant and QString here
      DataRow headerRow;
      if (m_config.hasHeader) {
        headerRow.values.reserve(fieldCount);
        for (quint32 f = fBegin; f < fEnd; ++f) {
          const std::string_view sv = batch.field(f);
          headerRow.append(QVariant(QString::fromUtf8(sv.data(), sv.size())));
        }
      } else {
        headerRow.values.assign(fieldCount, QVariant(QString()));
      }

      auto result = parseHeaderRow(headerRow);
      if (!result) {
        fail(std::move(QString::fromUtf8(result.error().c_str())));
        return;
      }

      // Columns are now known: resolve pointers and size staging buffers.
      prepareFastStaging();
      for (auto &col : m_colValues) {
        col.reserve(rowCount);
      }

      if (m_config.hasHeader) {
        m_currentPhysicalRow++;
        continue;
      }
      // else: this row is data — fall through.
    }

    // rowIsValid analog: column count must match the resolved schema.
    if (m_columnSize < 0 || fieldCount != m_columnSize) {
      m_currentPhysicalRow++;
      continue;
    }

    for (qint64 ci = 0; ci < m_columnSize; ++ci) {
      MutableDataColumn *col = m_colPtrs[ci];
      double value = std::numeric_limits<double>::quiet_NaN();
      if (col) {
        convertSpanValue(*col, batch.field(fBegin + ci), value);
      }
      m_colValues[ci].push_back(value);
    }
    ++accepted;
    m_currentPhysicalRow++;
  }

  m_convertNs += m_profTimer.nsecsElapsed() - convStart;

  if (accepted > 0) {
    const qint64 t1 = m_profTimer.nsecsElapsed();
    m_buffer->appendColumnsBulk(m_colValues, accepted);
    m_appendNs += m_profTimer.nsecsElapsed() - t1;
  }

  emit bufferUpdated();
}

void DataBufferUpdater::prepareFastStaging() {
  if (m_columnSize <= 0) {
    return;
  }

  if (static_cast<qint64>(m_colPtrs.size()) != m_columnSize) {
    m_colPtrs.assign(m_columnSize, nullptr);
    for (qint64 ci = 0; ci < m_columnSize; ++ci) {
      auto column = m_buffer->column(ci);
      m_colPtrs[ci] = column.has_value() ? &column->get() : nullptr;
    }
  }

  if (m_colValues.size() != m_columnSize) {
    m_colValues.resize(m_columnSize);
  }
  for (auto &col : m_colValues) {
    col.resize(0);
  }
}

bool DataBufferUpdater::convertSpanValue(MutableDataColumn &col,
                                         std::string_view field,
                                         double &outValue) {
  // Type inference (first time the column type is Unknown) needs the text form.
  if (col.type == ChartEnums::DataType::Unknown && m_canInferTypes) {
    const QVariant probe(QString::fromUtf8(field.data(), field.size()));
    double dateProbe = 0.0;
    if (Utils::Variant::isDouble(probe)) {
      col.type = ChartEnums::DataType::Number;
    } else if (Utils::Variant::isDate(probe) || refDateMs(field, dateProbe)) {
      // refDateMs also catches space-separated timestamps that Qt's QVariant
      // path misses — otherwise they would fall into slow string-interning.
      col.type = ChartEnums::DataType::Date;
    } else {
      col.type = ChartEnums::DataType::String;
    }
  }

  switch (col.type) {
  case ChartEnums::DataType::Number: {
    const char *first = field.data();
    const char *last = first + field.size();
    while (first < last && (*first == ' ' || *first == '\t')) {
      ++first;
    }
    while (last > first && (last[-1] == ' ' || last[-1] == '\t')) {
      --last;
    }
    if (first < last && *first == '+') {
      ++first; // std::from_chars rejects a leading '+'
    }

    double parsed = 0.0;
    const auto [ptr, ec] = std::from_chars(first, last, parsed);
    outValue = (ec == std::errc() && ptr == last)
                   ? parsed
                   : std::numeric_limits<double>::quiet_NaN();
    break;
  }
  case ChartEnums::DataType::Date: {
    // Decide the parse strategy from the first value, validated against the
    // original QVariant result so epoch-ms (hence x-axis values) are identical.
    if (col.dateParseMode == 0) {
      double ref = 0.0;
      if (!refDateMs(field, ref)) {
        outValue = std::numeric_limits<double>::quiet_NaN();
        break; // leave mode undetermined; retry on the next value
      }
      double probe = 0.0;
      if (fastDateMs(field, probe) && probe == ref) {
        col.dateParseMode = 1;
      } else if (isoDateMs(field, probe) && probe == ref) {
        col.dateParseMode = 2;
      } else {
        col.dateParseMode = 3;
      }
      outValue = ref;
      break;
    }

    double ms = 0.0;
    bool ok = false;
    if (col.dateParseMode == 1) {
      ok = fastDateMs(field, ms);
    }
    if (!ok && col.dateParseMode <= 2) {
      ok = isoDateMs(field, ms);
    }
    if (!ok) {
      ok = refDateMs(field, ms); // correctness backstop for odd rows
    }
    outValue = ok ? ms : std::numeric_limits<double>::quiet_NaN();
    break;
  }
  case ChartEnums::DataType::String:
  default: {
    if (field.empty()) {
      outValue = std::numeric_limits<double>::quiet_NaN();
      break;
    }
    const QString text = QString::fromUtf8(field.data(), field.size());
    const auto it = col.stringToId.constFind(text);
    if (it == col.stringToId.constEnd()) {
      const double newId = static_cast<double>(col.idToString.size());
      col.stringToId.insert(text, newId);
      col.idToString.push_back(text);
      outValue = newId;
    } else {
      outValue = it.value();
    }
    break;
  }
  }

  return true;
}

std::expected<void, std::string>
DataBufferUpdater::parseRow(const DataRow &row) {
  assert(m_buffer != nullptr);

  if (m_currentPhysicalRow < m_config.skipRows) {
    return {};
  }
  // CP_DEBUG(row.toString());

  if (!m_firstRowParsed) {
    m_firstRowParsed = true;
    if (m_config.hasHeader) {
      return parseHeaderRow(row);
    }

    DataRow emptyHeaderRow;
    emptyHeaderRow.values.assign(row.values.size(), QString());
    auto result = parseHeaderRow(emptyHeaderRow);
    if (!result) {
      return result;
    }
  }

  assert(m_columnSize != -1);
  if (!rowIsValid(row)) {
    return {};
  }

  QVector<double> convertedRow;
  convertedRow.reserve(m_columnSize);

  for (qint64 i = 0; i < m_columnSize; ++i) {
    auto value = row.values.at(i);
    auto result = m_buffer->column(i);
    if (!result.has_value()) {
      return {};
    }

    auto &column = result->get();
    double doubleVal = std::numeric_limits<double>::quiet_NaN();

    if (!convertValue(column, value, doubleVal)) {
      return {};
    }

    convertedRow.push_back(doubleVal);
  }

  m_buffer->appendRow(convertedRow);

  return {};
}

std::expected<void, std::string>
DataBufferUpdater::parseHeaderRow(const DataRow &headerRow) {
  assert(m_buffer != nullptr);

  const auto defaultType = m_canInferTypes ? ChartEnums::DataType::Unknown
                                           : ChartEnums::DataType::String;

  const qsizetype headerSize = headerRow.size();

  qsizetype finalSize = headerSize;

  if (m_config.totalColumns > 0) {
    finalSize = qsizetype(m_config.totalColumns);
  } else {
    for (const auto &schema : m_config.columns) {
      finalSize = std::max(finalSize, qsizetype(schema.idx + 1));
    }
  }

  enum class ColumnNameSource {
    Header,
    Default,
    Schema,
  };

  struct ColumnNameInfo {
    qsizetype idx;
    ColumnNameSource source;
  };

  auto defaultColumnName = [](qsizetype i) {
    return QStringLiteral("column") + QString::number(i);
  };

  auto sourceText = [](ColumnNameSource source) {
    switch (source) {
    case ColumnNameSource::Header:
      return QStringLiteral("header");
    case ColumnNameSource::Default:
      return QStringLiteral("default column name");
    case ColumnNameSource::Schema:
      return QStringLiteral("column schema");
    }

    return QStringLiteral("unknown");
  };

  QVector<ColumnInitField> colInitFields;
  QVector<ColumnNameSource> nameSources;

  colInitFields.reserve(finalSize);
  nameSources.reserve(finalSize);

  const qsizetype usedHeaderSize = std::min(headerSize, finalSize);

  qsizetype i = 0;

  for (; i < usedHeaderSize; ++i) {
    QString headerName = headerRow.values.at(i).toString();
    ColumnNameSource source = ColumnNameSource::Header;

    if (headerName.isEmpty()) {
      headerName = defaultColumnName(i);
      source = ColumnNameSource::Default;
    }

    colInitFields.push_back(ColumnInitField{
        .name = std::move(headerName),
        .ty = defaultType,
    });
    nameSources.push_back(source);
  }

  for (; i < finalSize; ++i) {
    colInitFields.push_back(ColumnInitField{
        .name = defaultColumnName(i),
        .ty = defaultType,
    });
    nameSources.push_back(ColumnNameSource::Default);
  }

  for (const auto &schema : m_config.columns) {
    if (schema.idx < 0 || qsizetype(schema.idx) >= finalSize) {
      return std::unexpected(
          QStringLiteral("Column schema index %1 exceeds total column count %2")
              .arg(schema.idx)
              .arg(finalSize)
              .toStdString());
    }

    auto &field = colInitFields[schema.idx];

    if (!schema.name.isEmpty()) {
      field.name = schema.name;
      nameSources[schema.idx] = ColumnNameSource::Schema;
    }

    if (schema.type != ChartEnums::DataType::Unknown) {
      field.ty = schema.type;
    }
  }

  QHash<QString, ColumnNameInfo> usedColumnNames;
  usedColumnNames.reserve(finalSize);

  for (qsizetype idx = 0; idx < finalSize; ++idx) {
    const auto &name = colInitFields[idx].name;

    const auto existingIt = usedColumnNames.constFind(name);
    if (existingIt != usedColumnNames.constEnd()) {
      const auto &existing = existingIt.value();

      return std::unexpected(
          QStringLiteral(
              "Column name '%1' cannot be duplicated. It is used by column %2 "
              "from %3 and column %4 from %5.")
              .arg(name)
              .arg(existing.idx)
              .arg(sourceText(existing.source))
              .arg(idx)
              .arg(sourceText(nameSources[idx]))
              .toStdString());
    }

    usedColumnNames.insert(name, ColumnNameInfo{
                                     .idx = idx,
                                     .source = nameSources[idx],
                                 });
  }

  m_columnSize = finalSize;
  m_buffer->initColumns(std::move(colInitFields));

  return {};
}

bool DataBufferUpdater::rowIsValid(const DataRow &row) {
  if (row.isEmpty()) {
    return false;
  }

  for (const auto &val : row.values) {
    if (!val.isValid()) {
      return false;
    }
  }

  if (row.size() != m_columnSize) {
    return false;
  }

  return true;
}

bool DataBufferUpdater::convertValue(MutableDataColumn &col,
                                     const QVariant &val, double &outValue) {
  QString valStr = val.toString();

  // Initial Type Inference (Only happens if type is Unknown)
  if (col.type == ChartEnums::DataType::Unknown && m_canInferTypes) {
    if (Utils::Variant::isDouble(valStr)) {
      col.type = ChartEnums::DataType::Number;
    } else if (Utils::Variant::isDate(valStr)) {
      col.type = ChartEnums::DataType::Date;
    } else {
      col.type = ChartEnums::DataType::String;
    }
  }

  switch (col.type) {
  case ChartEnums::DataType::Number: {
    bool ok;
    outValue = valStr.toDouble(&ok);
    if (!ok) {
      // Bad numeric data. Instead of downgrading the whole column,
      // we just leave outValue as NaN. The chart will simply skip rendering
      // this point.
      outValue = std::numeric_limits<double>::quiet_NaN();
    }
    break;
  }
  case ChartEnums::DataType::Date: {
    if (!Utils::Variant::variantToDateNumber(val, outValue)) {
      outValue = std::numeric_limits<double>::quiet_NaN();
    }
    break;
  }
  case ChartEnums::DataType::String:
  default: {
    if (valStr.isEmpty()) {
      outValue = std::numeric_limits<double>::quiet_NaN();
      break;
    }
    if (!col.stringToId.contains(valStr)) {
      double newId = static_cast<double>(col.idToString.size());
      col.stringToId.insert(valStr, newId);
      col.idToString.push_back(valStr);
    }
    outValue = col.stringToId.value(valStr);
    break;
  }
  }

  return true;
}

void DataBufferUpdater::fail(QString &&message) {
  if (m_buffer) {
    m_buffer->setIsValid(false);
  }
  emit errorOccurred(std::move(message));
}

} // namespace ChartPlotter
