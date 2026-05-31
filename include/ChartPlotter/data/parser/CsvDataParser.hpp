#include "ChartPlotter/data/parser/AbstractDataParser.hpp"

namespace ChartPlotter {

class CsvDataParser;

class ICsvParserState {
public:
  virtual ~ICsvParserState() = default;

  virtual void handleChar(CsvDataParser *context, int c) = 0;
};

class CsvErrorState : public ICsvParserState {
public:
  void handleChar(CsvDataParser *context, int c) override;
};
class CsvWithinUnquotedFieldState : public ICsvParserState {
public:
  void handleChar(CsvDataParser *context, int c) override;
};
class CsvAfterQuoteWithinQuotedFieldState : public ICsvParserState {
public:
  void handleChar(CsvDataParser *context, int c) override;
};
class CsvWithinQuotedFieldState : public ICsvParserState {
public:
  void handleChar(CsvDataParser *context, int c) override;
};
class CsvAfterFieldEndState : public ICsvParserState {
public:
  void handleChar(CsvDataParser *context, int c) override;
};
class CsvAfterRecordEndState : public ICsvParserState {
public:
  void handleChar(CsvDataParser *context, int c) override;
};

class CsvDataParser : public AbstractDataParser {
  Q_OBJECT

public:
  explicit CsvDataParser(QObject *parent = nullptr);

  void parse(const QByteArray &chunk) override;
  void reset() override;
  void done();

  void transitionTo(ICsvParserState *newState);
  ICsvParserState *csvErrorState();
  ICsvParserState *csvWithinUnquotedFieldState();
  ICsvParserState *csvAfterQuoteWithinQuotedFieldState();
  ICsvParserState *csvWithinQuotedFieldState();
  ICsvParserState *csvAfterFieldEndState();
  ICsvParserState *csvAfterRecordEndState();
  void bufferByte(char c);
  void clearPendingBytes();
  bool isPendingBytesEmpty() const;
  void commitCurrentPendingBytes();
  void commitCurrentRow();
  void emitRowsIfAny();

private:
  DataRow m_currentRow;
  QVector<DataRow> m_rows;
  ICsvParserState *m_currentState = &m_csvAfterRecordEndState;
  QByteArray m_pendingBytes;
  QVector<QString> m_headerNames;
  int m_currentColumnIndex = 0;
  int m_currentRowIndex = 0;
  bool m_isDone = false;
  CsvErrorState m_csvErrorState;
  CsvWithinUnquotedFieldState m_csvWithinUnquotedFieldState;
  CsvAfterQuoteWithinQuotedFieldState m_csvAfterQuoteWithinQuotedFieldState;
  CsvWithinQuotedFieldState m_csvWithinQuotedFieldState;
  CsvAfterFieldEndState m_csvAfterFieldEndState;
  CsvAfterRecordEndState m_csvAfterRecordEndState;
};

} // namespace ChartPlotter
