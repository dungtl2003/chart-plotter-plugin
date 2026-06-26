#include "ChartPlotter/data/reader/DataReaderFactory.hpp"
#include "ChartPlotter/data/reader/FifoDataReader.hpp"
#include "ChartPlotter/data/reader/FileDataReader.hpp"
#include "ChartPlotter/data/reader/WebSocketDataReader.hpp"

namespace ChartPlotter {

AbstractDataReader *DataReaderFactory::create(const QUrl &url,
                                              QObject *parent) {
  const QString scheme = url.scheme().toLower();

  if (scheme == "file") {
    return new FileDataReader(parent);
  }

  if (scheme == "fifo" || scheme == "pipe") {
    return new FifoDataReader(parent);
  }

  if (scheme == "ws" || scheme == "wss") {
    return new WebSocketDataReader(parent);
  }

  return nullptr;
}

} // namespace ChartPlotter
