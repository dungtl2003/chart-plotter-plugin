#include "ChartPlotter/data/DataManagerPool.hpp"

namespace ChartPlotter {

DataManagerPool::DataManagerPool(QObject *parent) : QObject(parent) {}
DataManagerPool::~DataManagerPool() { shutdownDataManagers(); }

void DataManagerPool::onDataError(const QString &message) {
  emit errorOccurred(message);
}

void DataManagerPool::onSnapshotReady(int sourceId,
                                      const DataSnapshot &snapshot) {
  emit snapshotReady(sourceId, snapshot);
}

const QHash<DataSource *, int> &DataManagerPool::sourceIds() const {
  return m_sourceIds;
}

QPointer<DataManager>
DataManagerPool::createDataManager(const QPointer<DataSource> source) {
  const int id = m_nextSourceId++;

  m_sourceIds.insert(source.data(), id);

  QPointer<QThread> thread = new QThread(this);

  QPointer<DataManager> manager = new DataManager();
  manager->setDataReadConfig(std::move(source->exportConfig()));
  manager->moveToThread(thread);

  connect(thread, &QThread::started, manager, &DataManager::start);
  connect(thread, &QThread::finished, manager, &QObject::deleteLater);
  connect(thread, &QThread::finished, this, [this, manager, thread]() {
    this->shutdownDataManager(manager, thread);
  });
  connect(manager, &DataManager::errorOccurred, this,
          &DataManagerPool::onDataError, Qt::QueuedConnection);
  connect(
      manager, &DataManager::snapshotReady, this,
      [this, id](const DataSnapshot &snapshot) {
        onSnapshotReady(id, snapshot);
      },
      Qt::QueuedConnection);
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

} // namespace ChartPlotter
