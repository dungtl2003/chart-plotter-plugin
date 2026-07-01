#pragma once

#include "ChartPlotter/data/Column.hpp"
#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/data/DataReadConfig.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"

#include <QObject>
#include <QtQml>

namespace ChartPlotter {

class DataSource : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(qint64 totalColumns READ totalColumns WRITE setTotalColumns NOTIFY
                 totalColumnsChanged)
  Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
  Q_PROPERTY(ChartPlotter::ChartEnums::DataFormat format READ format WRITE
                 setFormat NOTIFY formatChanged)
  Q_PROPERTY(qint64 chunkSize READ chunkSize WRITE setChunkSize NOTIFY
                 chunkSizeChanged)
  Q_PROPERTY(QQmlListProperty<ChartPlotter::Column> columns READ columns NOTIFY
                 columnsChanged)
  Q_PROPERTY(
      bool hasHeader READ hasHeader WRITE setHasHeader NOTIFY hasHeaderChanged)
  Q_PROPERTY(
      int skipRows READ skipRows WRITE setSkipRows NOTIFY skipRowsChanged)

public:
  explicit DataSource(QObject *parent = nullptr);

  qint64 totalColumns() const;
  void setTotalColumns(qint64 newTotalColumns);
  QUrl url() const;
  void setUrl(const QUrl &newUrl);
  ChartEnums::DataFormat format() const;
  void setFormat(ChartEnums::DataFormat newFormat);
  qint64 chunkSize() const;
  void setChunkSize(qint64 newChunkSize);
  bool hasHeader() const;
  void setHasHeader(bool newHasHeader);
  int skipRows() const;
  void setSkipRows(int skipRows);

  QQmlListProperty<Column> columns();

  const QVector<Column *> &columnList() const;

  DataReadConfig exportConfig() const;

signals:
  void totalColumnsChanged();
  void urlChanged();
  void modeChanged();
  void formatChanged();
  void chunkSizeChanged();
  void columnsChanged();
  void hasHeaderChanged();
  void skipRowsChanged();

private:
  qint64 m_totalColumns = -1;
  QUrl m_url;
  ChartEnums::DataFormat m_format = ChartEnums::DataFormat::Unknown;
  qint64 m_chunkSize = 2 * ChartEnums::DataUnit::Mb;
  std::unique_ptr<DataBuffer> m_buffer;
  QVector<Column *> m_columns;
  bool m_hasHeader = false;
  int m_skipRows = 0;

  static void appendColumn(QQmlListProperty<Column> *list, Column *column);
  static qsizetype columnCount(QQmlListProperty<Column> *list);
  static Column *columnAt(QQmlListProperty<Column> *list, qsizetype index);
  static void clearColumns(QQmlListProperty<Column> *list);
};

} // namespace ChartPlotter
