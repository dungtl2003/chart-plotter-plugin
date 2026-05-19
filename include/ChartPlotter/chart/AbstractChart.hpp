#pragma once

#include <QObject>
#include <QtQml>

enum class ChartType {
  Line,
  Bar,
  Pie,
};

class AbstractChart : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("AbstractChart is a base class and cannot be instantiated.")

public:
  explicit AbstractChart(QObject *parent = nullptr);

  explicit AbstractChart(const AbstractChart &) = delete;
  explicit AbstractChart(AbstractChart &&) = delete;
  AbstractChart &operator=(const AbstractChart &) = delete;
  AbstractChart &operator=(AbstractChart &&) = delete;
  virtual ~AbstractChart() = default;

  virtual ChartType type() const = 0;
};
