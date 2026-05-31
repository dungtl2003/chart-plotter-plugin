#pragma once

#include "ChartPlotter/types/ChartEnums.hpp"

#include <QObject>
#include <QtQml>

namespace ChartPlotter {

class Column : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(int idx READ idx WRITE setIdx NOTIFY idxChanged REQUIRED)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY(ChartPlotter::ChartEnums::DataType type READ type WRITE setType
                 NOTIFY typeChanged)

public:
  explicit Column(QObject *parent = nullptr);

  int idx() const;
  void setIdx(int newIdx);

  QString name() const;
  void setName(const QString &newName);

  ChartEnums::DataType type() const;
  void setType(ChartEnums::DataType newType);

signals:
  void idxChanged();
  void nameChanged();
  void typeChanged();

private:
  int m_idx = -1;
  QString m_name;
  ChartEnums::DataType m_type = ChartEnums::DataType::Unknown;
};

} // namespace ChartPlotter
