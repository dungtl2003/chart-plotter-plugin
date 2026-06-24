#include "ChartPlotter/data/DataBufferUpdater.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"
#include "ChartPlotter/utils/Variant.hpp"

namespace ChartPlotter {

DataBufferUpdater::DataBufferUpdater(std::shared_ptr<DataBuffer> buffer,
                                     QObject *parent)
    : m_buffer(buffer), QObject(parent) {}

void DataBufferUpdater::setConfig(const DataBufferUpdaterConfig &config) {
  m_config = config;

  if (m_config.skipRows < 0) {
    emit errorOccurred(std::move(
        QStringLiteral("DataBufferUpdater::setConfig: skipRows cannot be "
                       "negative, set default to 0")));
    m_config.skipRows = 0;
  }

  if (m_config.mode == ChartEnums::DataMode::Static) {
    m_canInferTypes = true;
  }

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
