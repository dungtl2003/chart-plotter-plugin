#pragma once
#include <QObject>
#include <QString>
#include <QtQml>

class ChartModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString provider READ provider CONSTANT)

public:
  explicit ChartModel(QObject *parent = nullptr);
  QString provider() const;
};
