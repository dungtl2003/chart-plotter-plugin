#include "ChartPlotter/data/parser/FastCsvDataParser.hpp"

namespace ChartPlotter {

FastCsvDataParser::FastCsvDataParser(QObject *parent)
    : AbstractDataParser(parent) {}

void FastCsvDataParser::parse(const QByteArray &chunk) {
  if (chunk.isEmpty()) {
    return;
  }

  if (m_carry.isEmpty()) {
    emitFromBuffer(chunk, false);
  } else {
    QByteArray buf = m_carry;
    buf.append(chunk);
    m_carry.clear();
    emitFromBuffer(buf, false);
  }
}

void FastCsvDataParser::done() {
  if (!m_carry.isEmpty()) {
    const QByteArray buf = m_carry;
    m_carry.clear();
    emitFromBuffer(buf, true);
  }
  emit finished();
}

void FastCsvDataParser::reset() { m_carry.clear(); }

void FastCsvDataParser::emitFromBuffer(const QByteArray &buf, bool final) {
  const char *data = buf.constData();
  const qsizetype size = buf.size();

  CsvRowBatch batch;
  // Rough reservations to avoid reallocation churn while scanning. Field count
  // is bounded by `size`; ~size/4 covers realistic CSV (>=2-byte fields)
  // without over-allocating, and the vectors are freed when the batch is
  // consumed.
  batch.storage.reserve(static_cast<int>(size));
  const qsizetype fieldGuess = size / 4 + 16;
  batch.fieldStart.reserve(fieldGuess);
  batch.fieldLen.reserve(fieldGuess);
  batch.rowStart.reserve(size / 16 + 16); // guess 4 values per row

  qsizetype i = 0;
  while (i < size) {
    // Skip blank lines / stray newlines between records.
    if (data[i] == '\n') {
      ++i;
      continue;
    }
    if (data[i] == '\r') {
      ++i;
      if (i < size && data[i] == '\n') {
        ++i;
      }
      continue;
    }

    const qsizetype recordStart = i;
    const qsizetype firstField = batch.fieldStart.size();
    const qsizetype storageStart = batch.storage.size();

    if (!parseRecord(data, size, i, batch, final)) {
      // Incomplete record at the end of this chunk: undo its partial fields and
      // carry the raw bytes into the next chunk.
      batch.fieldStart.resize(firstField);
      batch.fieldLen.resize(firstField);
      batch.storage.resize(storageStart);
      m_carry = buf.mid(static_cast<int>(recordStart));
      break;
    }

    // Commit the record (skip records that produced no fields).
    if (batch.fieldStart.size() > firstField) {
      batch.rowStart.append(static_cast<quint32>(firstField));
    }
  }

  if (!batch.rowStart.isEmpty()) {
    batch.rowStart.append(static_cast<quint32>(batch.fieldStart.size()));
    emit batchParsed(batch);
  }
}

bool FastCsvDataParser::parseRecord(const char *data, qsizetype size,
                                    qsizetype &i, CsvRowBatch &b,
                                    bool final) const {
  while (true) {
    const quint32 fStart = static_cast<quint32>(b.storage.size());

    if (i < size && data[i] == '"') {
      // Quoted field: copy unescaped bytes, "" -> ".
      ++i; // opening quote
      bool closed = false;
      while (i < size) {
        const char c = data[i];
        if (c == '"') {
          if (i + 1 < size) {
            if (data[i + 1] == '"') {
              b.storage.append('"');
              i += 2;
              continue;
            }
            ++i; // consume closing quote
            closed = true;
            break;
          }
          // Quote is the last byte: can't tell closing vs "" escape yet.
          if (final) {
            ++i;
            closed = true;
          }
          break;
        }
        b.storage.append(c);
        ++i;
      }
      if (!closed) {
        return false; // ran out mid-quote
      }
      b.fieldStart.append(fStart);
      b.fieldLen.append(static_cast<quint32>(b.storage.size()) - fStart);
    } else {
      // Unquoted field: find the delimiter, then bulk-copy the whole field in
      // one go (no per-byte append).
      const qsizetype start = i;
      while (i < size && data[i] != ',' && data[i] != '\n' && data[i] != '\r') {
        ++i;
      }
      const bool terminated = i < size;
      if (!terminated && !final) {
        return false; // field continues into the next chunk
      }
      b.storage.append(data + start, i - start);
      b.fieldStart.append(fStart);
      b.fieldLen.append(static_cast<quint32>(i - start));
      if (!terminated) {
        return true; // EOF terminates the field/record
      }
    }

    // After a field we are at ',' or a newline (or EOF after a quoted field).
    if (i >= size) {
      return final; // record ends at EOF if this is the final flush
    }

    const char c = data[i];
    if (c == ',') {
      ++i;
      continue; // next field
    }
    if (c == '\n') {
      ++i;
      return true;
    }
    if (c == '\r') {
      ++i;
      if (i < size) {
        if (data[i] == '\n') {
          ++i;
        }
        return true;
      }
      // CR at the very end: a following LF might arrive next chunk.
      return final;
    }

    // Stray byte right after a closing quote (malformed). Skip it.
    ++i;
  }
}

} // namespace ChartPlotter
