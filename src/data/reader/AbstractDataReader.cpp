#include "ChartPlotter/data/reader/AbstractDataReader.hpp"

namespace ChartPlotter {

AbstractDataReader::AbstractDataReader(QObject *parent) : QObject(parent) {}

void AbstractDataReader::setQueueCapacity(int capacity) {
  m_queueCapacity = capacity;
  if (capacity > 0) {
    m_freeSlots.release(capacity);
  }
}

void AbstractDataReader::emitChunk(QByteArray &&chunk) {
  if (m_queueCapacity > 0) {
    // Wait for a free slot, waking every 50 ms so a stop arriving while the
    // queue is full is honored without depending on a separate wake signal.
    while (!m_freeSlots.tryAcquire(1, 50)) {
      if (m_stopRequested.load()) {
        return;
      }
    }
    if (m_stopRequested.load()) {
      m_freeSlots.release();
      return;
    }
  }

  emit dataReceived(std::move(chunk));
}

void AbstractDataReader::releaseChunkSlot() {
  if (m_queueCapacity > 0) {
    m_freeSlots.release();
  }
}

void AbstractDataReader::requestStop() { m_stopRequested.store(true); }

} // namespace ChartPlotter
