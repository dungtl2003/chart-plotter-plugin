#include "ChartPlotter/data/parser/CsvDataParser.hpp"

namespace ChartPlotter {

namespace {

const constexpr char Quote = '"';
const constexpr char Comma = ',';
const constexpr char LineFeed = '\n';
const constexpr char CarriageReturn = '\r';
const constexpr bool isNewLine(char c) {
  return c == LineFeed || c == CarriageReturn;
}

} // namespace

CsvDataParser::CsvDataParser(QObject *parent) : AbstractDataParser(parent) {}

void CsvDataParser::parse(const QByteArray &chunk) {
  for (int c : chunk) {
    m_currentState->handleChar(this, c);
    if (m_isDone) {
      m_isDone = false;
      return;
    }
  }

  emitRowsIfAny();
}

void CsvDataParser::done() {
  m_isDone = true;
  emitRowsIfAny();
  emit finished();
}

void CsvDataParser::reset() {
  m_isDone = false;
  m_pendingBytes.clear();
  m_currentRow.clear();
  m_rows.clear();
  m_currentState = &m_csvAfterRecordEndState;
  m_headerNames.clear();
  m_currentColumnIndex = 0;
  m_currentRowIndex = 0;
}

void CsvErrorState::handleChar(CsvDataParser *context, int c) {
  context->clearPendingBytes();

  if (c == EOF) {
    context->done();
  }

  if (isNewLine(c)) {
    context->transitionTo(context->csvAfterRecordEndState());
  }
}

void CsvAfterRecordEndState::handleChar(CsvDataParser *context, int c) {
  if (!context->isPendingBytesEmpty()) {
    context->transitionTo(context->csvErrorState());
    return;
  }

  if (c == EOF) {
    context->commitCurrentRow();
    context->done();
    return;
  }

  if (c == Quote) {
    context->transitionTo(context->csvWithinQuotedFieldState());
    return;
  }

  if (c == Comma) {
    context->commitCurrentPendingBytes();
    context->transitionTo(context->csvAfterFieldEndState());
    return;
  }

  if (isNewLine(c)) {
    return;
  }

  context->bufferByte(c);
  context->transitionTo(context->csvWithinUnquotedFieldState());
}

void CsvWithinQuotedFieldState::handleChar(CsvDataParser *context, int c) {
  if (c == EOF) {
    context->transitionTo(context->csvErrorState());
    context->done();
    return;
  }

  if (c == Quote) {
    context->transitionTo(context->csvAfterQuoteWithinQuotedFieldState());
    return;
  }

  context->bufferByte(c);
}

void CsvAfterQuoteWithinQuotedFieldState::handleChar(CsvDataParser *context,
                                                     int c) {
  if (c == EOF) {
    context->commitCurrentRow();
    context->done();
    return;
  }

  if (c == Quote) {
    context->bufferByte(c);
    context->transitionTo(context->csvWithinQuotedFieldState());
    return;
  }

  if (c == Comma) {
    context->commitCurrentPendingBytes();
    context->transitionTo(context->csvAfterFieldEndState());
    return;
  }

  if (isNewLine(c)) {
    context->commitCurrentRow();
    context->transitionTo(context->csvAfterRecordEndState());
    return;
  }

  context->transitionTo(context->csvErrorState());
}

void CsvAfterFieldEndState::handleChar(CsvDataParser *context, int c) {
  if (c == EOF) {
    context->commitCurrentRow();
    context->done();
    return;
  }

  if (c == Quote) {
    context->transitionTo(context->csvWithinQuotedFieldState());
    return;
  }

  if (c == Comma) {
    context->commitCurrentPendingBytes();
    return;
  }

  if (isNewLine(c)) {
    context->commitCurrentRow();
    context->transitionTo(context->csvAfterRecordEndState());
    return;
  }

  context->bufferByte(c);
  context->transitionTo(context->csvWithinUnquotedFieldState());
}

void CsvWithinUnquotedFieldState::handleChar(CsvDataParser *context, int c) {
  if (c == EOF) {
    context->commitCurrentPendingBytes();
    context->commitCurrentRow();
    context->done();
    return;
  }

  if (c == Quote) {
    context->transitionTo(context->csvErrorState());
    return;
  }

  if (c == Comma) {
    context->commitCurrentPendingBytes();
    context->transitionTo(context->csvAfterFieldEndState());
    return;
  }

  if (isNewLine(c)) {
    context->commitCurrentPendingBytes();
    context->commitCurrentRow();
    context->transitionTo(context->csvAfterRecordEndState());
    return;
  }

  context->bufferByte(c);
}

void CsvDataParser::transitionTo(ICsvParserState *newState) {
  m_currentState = newState;
}

ICsvParserState *CsvDataParser::csvErrorState() { return &m_csvErrorState; }
ICsvParserState *CsvDataParser::csvWithinUnquotedFieldState() {
  return &m_csvWithinUnquotedFieldState;
}
ICsvParserState *CsvDataParser::csvAfterQuoteWithinQuotedFieldState() {
  return &m_csvAfterQuoteWithinQuotedFieldState;
}
ICsvParserState *CsvDataParser::csvWithinQuotedFieldState() {
  return &m_csvWithinQuotedFieldState;
}
ICsvParserState *CsvDataParser::csvAfterFieldEndState() {
  return &m_csvAfterFieldEndState;
}
ICsvParserState *CsvDataParser::csvAfterRecordEndState() {
  return &m_csvAfterRecordEndState;
}

void CsvDataParser::bufferByte(char c) { m_pendingBytes.push_back(c); }

void CsvDataParser::clearPendingBytes() { m_pendingBytes.clear(); }

bool CsvDataParser::isPendingBytesEmpty() const {
  return m_pendingBytes.isEmpty();
}

void CsvDataParser::commitCurrentPendingBytes() {
  QVariant cell = m_pendingBytes.isEmpty()
                      ? QVariant()
                      : QVariant(QString::fromUtf8(m_pendingBytes));
  m_currentRow.append(cell);
  m_pendingBytes.clear();
}

void CsvDataParser::commitCurrentRow() {
  if (m_currentRow.isEmpty()) {
    return;
  }

  m_rows.append(m_currentRow);
  m_currentRow.clear();
}

void CsvDataParser::emitRowsIfAny() {
  if (!m_rows.isEmpty()) {
    emit rowsParsed(std::move(m_rows));
    m_rows.clear();
  }
}

} // namespace ChartPlotter
