#include "ChartPlotter/data/DataManagerPool.hpp"

namespace ChartPlotter {

DataManagerPool::DataManagerPool(QObject *parent) : QObject(parent) {
  m_fpsTimer = new QTimer(this);
  m_fpsTimer->setInterval(1000 / m_fps);
  connect(m_fpsTimer, &QTimer::timeout, this,
          &DataManagerPool::onFpsTimerTicked);
  m_fpsTimer->start();
}

DataManagerPool::~DataManagerPool() { shutdownDataManagers(); }

void DataManagerPool::onDataError(const QString &message) {
  emit errorOccurred(message);
}

void DataManagerPool::onSnapshotReady(int sourceId,
                                      const DataSnapshot &snapshot) {
  emit snapshotReady(sourceId, snapshot);
}

void snapshotsReady(
    std::vector<std::pair<int, const DataSnapshot &>> snapshots) {
  emit snapshotsReady(snapshots);
}

const QHash<DataSource *, int> &DataManagerPool::sourceIds() const {
  return m_sourceIds;
}

void DataManagerPool::setMaxRows(qint64 maxRows) {
  m_maxRows = maxRows;
  for (const DataManagerRuntime &runtime : m_dataManagers) {
    if (runtime.manager) {
      QMetaObject::invokeMethod(runtime.manager, "setMaxRows",
                                Qt::QueuedConnection, Q_ARG(qint64, maxRows));
    }
  }
}

QPointer<DataManager>
DataManagerPool::createDataManager(const QPointer<DataSource> source) {
  const int id = m_nextSourceId++;

  m_sourceIds.insert(source.data(), id);

  QPointer<QThread> thread = new QThread(this);

  QPointer<DataManager> manager = new DataManager();
  manager->setDataReadConfig(std::move(source->exportConfig()));
  manager->setSourceId(id);
  // Apply the current cap before the thread starts so start() configures the
  // buffer with it (direct call is safe here — not yet on the worker thread).
  manager->setMaxRows(m_maxRows);
  manager->moveToThread(thread);

  connect(thread, &QThread::started, manager, &DataManager::start);
  connect(thread, &QThread::finished, manager, &QObject::deleteLater);
  connect(thread, &QThread::finished, this, [this, manager, thread]() {
    this->shutdownDataManager(manager, thread);
  });
  connect(manager, &DataManager::errorOccurred, this,
          &DataManagerPool::onDataError, Qt::QueuedConnection);
  // connect(
  //     manager, &DataManager::snapshotReady, this,
  //     [this, id](const DataSnapshot &snapshot) {
  //       onSnapshotReady(id, snapshot);
  //     },
  //     Qt::QueuedConnection);
  connect(manager, &DataManager::finished, thread, &QThread::quit,
          Qt::QueuedConnection);

  m_dataManagers.push_back({id, thread, manager});

  thread->start();

  return manager;
}

void DataManagerPool::shutdownDataManagers() {
  for (DataManagerRuntime &runtime : m_dataManagers) {
    if (!runtime.manager) {
      continue;
    }

    QPointer<DataManager> manager = runtime.manager;
    QMetaObject::invokeMethod(manager, "stop", Qt::BlockingQueuedConnection);

    if (runtime.thread) {
      runtime.thread->quit();
      runtime.thread->wait();
    }

    runtime.manager = nullptr;
    runtime.thread = nullptr;
  }

  m_dataManagers.clear();
}

void DataManagerPool::shutdownDataManager(QPointer<DataManager> manager,
                                          QPointer<QThread> thread) {
  for (auto &runtime : m_dataManagers) {
    if (runtime.manager == manager) {
      QMetaObject::invokeMethod(manager, "stop", Qt::BlockingQueuedConnection);

      runtime.manager = nullptr;
      runtime.thread = nullptr;
      break;
    }
  }

  thread->deleteLater();
}

void DataManagerPool::onFpsTimerTicked() {
  std::vector<std::pair<int, DataSnapshot>> snapshots;

  for (auto &runtime : m_dataManagers) {
    if (!runtime.manager) {
      continue;
    }

    QPointer<DataManager> manager = runtime.manager;
    auto snapshot = manager->getLatestSnapshot();
    int sourceId = manager->getSourceId();
    snapshots.emplace_back(sourceId, snapshot);
  }

  if (snapshots.size() > 0) {
    emit snapshotsReady(std::move(snapshots));
  }
}

} // namespace ChartPlotter
