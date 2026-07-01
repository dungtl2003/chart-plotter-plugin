#pragma once

#include "ChartPlotter/types/ChartEnums.hpp"

#include <QObject>
#include <QSemaphore>
#include <QString>
#include <QtQml>

#include <atomic>

namespace ChartPlotter {

struct DataReaderConfig {
  QUrl url;
  qint64 chunkSize = 2 * ChartEnums::DataUnit::Mb;
  // Bounded-queue depth (max chunks emitted but not yet consumed) when the
  // reader runs on its own thread. <= 0 disables backpressure (unbounded).
  int maxQueuedChunks = 0;
};

class AbstractDataReader : public QObject {
  Q_OBJECT
  QML_UNCREATABLE("AbstractChart is a base class and cannot be instantiated.")

public:
  explicit AbstractDataReader(QObject *parent = nullptr);

  explicit AbstractDataReader(const AbstractDataReader &) = delete;
  explicit AbstractDataReader(AbstractDataReader &&) = delete;
  AbstractDataReader &operator=(const AbstractDataReader &) = delete;
  AbstractDataReader &operator=(AbstractDataReader &&) = delete;
  virtual ~AbstractDataReader() = default;

  virtual void setConfig(const DataReaderConfig &config) = 0;
  virtual ChartEnums::DataMode mode() const = 0;

  // Called by the downstream consumer (on its own thread) once a chunk emitted
  // via emitChunk() has been fully processed, freeing one queue slot so a
  // blocked reader can produce the next chunk. No-op when backpressure is off.
  void releaseChunkSlot();

  // Thread-safe stop request: only flips the atomic stop flag, so it is safe to
  // invoke from another thread (DirectConnection) and interrupts a blocking read
  // loop at once. Any thread-affine teardown (e.g. closing a socket) belongs in
  // the concrete stop() override, which the owner invokes on the reader thread.
  void requestStop();

public slots:
  virtual void start() = 0;
  virtual void stop() = 0;

signals:
  void dataReceived(QByteArray data);
  void started();
  void finished();
  void stopped();
  void errorOccurred(QString message);

protected:
  // Emit `chunk` to the consumer with backpressure. When a bounded queue depth
  // is configured, block until a slot is free (polling so a stop request is
  // still honored promptly) before emitting. Readers running on their own
  // thread should call this instead of `emit dataReceived` so a slow consumer
  // throttles the reader instead of letting chunks pile up unbounded.
  void emitChunk(QByteArray &&chunk);
  // Grant `capacity` in-flight slots. <= 0 disables backpressure. Call once at
  // config time, before start().
  void setQueueCapacity(int capacity);

  // Set by stop() in concrete readers; checked by the read loop and emitChunk
  // so a blocked reader unblocks and exits promptly.
  std::atomic_bool m_stopRequested{false};

private:
  QSemaphore m_freeSlots{0};
  int m_queueCapacity = 0;
};

} // namespace ChartPlotter
