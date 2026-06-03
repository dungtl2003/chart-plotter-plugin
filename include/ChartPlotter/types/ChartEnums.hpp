#pragma once

#include <QObject>
#include <QtQml>

namespace ChartPlotter {

class ChartEnums : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("ChartEnums is only used for enum access")

public:
  enum class SeriesType { Line, Bar, Pie };
  Q_ENUM(SeriesType)

  enum class LineStyle { Solid, Dashed, Dotted };
  Q_ENUM(LineStyle)

  enum class DataMode { Static, Realtime };
  Q_ENUM(DataMode)

  enum class DataFormat { Unknown, Csv, Tsv, Xlsx };
  Q_ENUM(DataFormat)

  enum class InteractionMode { None, Drag, Zoom, Crop, Pointing, Clear };
  Q_ENUM(InteractionMode)

  enum DataUnit { B = 1, Kb = 1024, Mb = 1024 * 1024, Gb = 1024 * 1024 * 1024 };
  Q_ENUM(DataUnit)

  enum class DataType {
    Unknown,
    String,
    Number,
    Date,
    Category,
  };
  Q_ENUM(DataType)
};

} // namespace ChartPlotter
