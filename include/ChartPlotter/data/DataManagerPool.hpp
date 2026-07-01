#pragma once

#include "ChartPlotter/data/DataManager.hpp"
#include "ChartPlotter/data/DataSource.hpp"

namespace ChartPlotter {

struct DataManagerRuntime {
  int id = -1;
  QPointer<QThread> thread = nullptr;
  QPointer<DataManager> manager = nullptr;
};

class DataManagerPool : public QObject {
  Q_OBJECT

public:
  explicit DataManagerPool(QObject *parent = nullptr);
  ~DataManagerPool();
  explicit DataManagerPool(const DataManagerPool &) = delete;
  explicit DataManagerPool(DataManagerPool &&) = delete;
  DataManagerPool &operator=(const DataManagerPool &) = delete;
  DataManagerPool &operator=(DataManagerPool &&) = delete;

  const QHash<DataSource *, int> &sourceIds() const;
  QPointer<DataManager> createDataManager(const QPointer<DataSource> source);
  // Global memory cap (rows) applied to every source's buffer. <= 0 = unlimited.
  // Propagated to existing managers on their worker threads (queued) and to any
  // managers created afterwards.
  void setMaxRows(qint64 maxRows);
  void shutdownDataManagers();
  void shutdownDataManager(QPointer<DataManager> manager,
                           QPointer<QThread> thread);

public slots:
  void onDataError(const QString &message);
  void onSnapshotReady(int sourceId, const DataSnapshot &snapshot);

signals:
  void errorOccurred(QString message);
  void snapshotReady(int sourceId, const DataSnapshot &snapshot);
  void snapshotsReady(std::vector<std::pair<int, DataSnapshot>> snapshots);

private slots:
  void onFpsTimerTicked();

private:
  QHash<DataSource *, int> m_sourceIds;
  QVector<DataManagerRuntime> m_dataManagers;
  int m_nextSourceId = 0;
  qint64 m_maxRows = -1; // global row cap; <= 0 = unlimited

  float m_fps = 60;
  QTimer *m_fpsTimer = nullptr;
};

} // namespace ChartPlotter
