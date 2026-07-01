#pragma once

#include "ChartPlotter/data/CsvRowBatch.hpp"
#include "ChartPlotter/data/parser/AbstractDataParser.hpp"

namespace ChartPlotter {

// High-throughput CSV parser for the static/file path.
//
// Unlike CsvDataParser (a per-byte virtual state machine that materialises every
// cell as a QString inside a QVariant), this scans each chunk with a flat loop
// and emits raw field byte-spans (CsvRowBatch). Conversion to double happens
// downstream via std::from_chars, so the common numeric case never allocates a
// QString/QVariant. Quoted fields ("" escaping, embedded commas/newlines) are
// still handled; an incomplete trailing record is carried into the next chunk.
class FastCsvDataParser : public AbstractDataParser {
  Q_OBJECT

public:
  explicit FastCsvDataParser(QObject *parent = nullptr);

  void parse(const QByteArray &chunk) override;
  void reset() override;
  void done();

signals:
  // Const-ref: consumed synchronously on the same thread (DirectConnection), so
  // the batch is never copied.
  void batchParsed(const CsvRowBatch &batch);

private:
  // Scans `buf` for complete records, emits a batch, and stores any incomplete
  // trailing record in m_carry. When `final` is true the trailing record is
  // treated as complete (file without a closing newline).
  void emitFromBuffer(const QByteArray &buf, bool final);

  // Parses one record starting at data[i], appending its fields to `batch`.
  // Returns true once the record's terminator (or EOF when `final`) is consumed;
  // false if the buffer ends mid-record (caller carries it to the next chunk).
  bool parseRecord(const char *data, qsizetype size, qsizetype &i,
                   CsvRowBatch &batch, bool final) const;

  QByteArray m_carry; // incomplete trailing record bytes
};

} // namespace ChartPlotter
