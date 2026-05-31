#pragma once

#include "ChartPlotter/data/DataRow.hpp"

#include <QtQml>

namespace ChartPlotter {

class AbstractDataParser : public QObject {
  Q_OBJECT
  QML_UNCREATABLE(
      "AbstractDataParser is a base class and cannot be instantiated.")

public:
  explicit AbstractDataParser(QObject *parent = nullptr);

public slots:
  virtual void parse(const QByteArray &chunk) = 0;
  virtual void reset() = 0;

signals:
  // we will clear rows right after this, so we need to pass by value
  void rowsParsed(QVector<DataRow> rows);
  void errorOccurred(QString message);
  void finished();
};

} // namespace ChartPlotter
