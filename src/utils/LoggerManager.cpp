#include "ChartPlotter/utils/LoggerManager.hpp"

#include "spdlog/sinks/null_sink.h"

#include <iostream>

std::shared_ptr<spdlog::sinks::stdout_color_sink_mt>
    LoggerManager::m_static_console_sink = nullptr;

std::shared_ptr<spdlog::sinks::basic_file_sink_mt>
    LoggerManager::m_static_file_sink = nullptr;

std::shared_ptr<spdlog::async_logger> LoggerManager::m_component_logger =
    nullptr;

void LoggerManager::init() {
  spdlog::level::level_enum level = spdlog::level::debug;

#ifdef NDEBUG
  // Default to info or warning for Release builds
  level = spdlog::level::info;
#endif

  // Override with environment variable if present
  if (const char *env_p = std::getenv("LOG_LEVEL")) {
    std::string env_level(env_p);
    if (env_level == "trace") {
      level = spdlog::level::trace;
    } else if (env_level == "debug") {
      level = spdlog::level::debug;
    } else if (env_level == "info") {
      level = spdlog::level::info;
    } else if (env_level == "warn") {
      level = spdlog::level::warn;
    } else if (env_level == "err") {
      level = spdlog::level::err;
    } else if (env_level == "critical") {
      level = spdlog::level::critical;
    } else if (env_level == "off") {
      level = spdlog::level::off;
    }
  }

  LoggerManager::init(Config{
      .log_level = level,
  });
}

void LoggerManager::init(Config config) {
  try {
    // Initialize the async thread pool
    // 8192 items in the lock-free queue, 1 dedicated background thread
    spdlog::init_thread_pool(8192, 1);

    // Create sinks (destinations for the log messages)
    // _mt denotes multi-thread safe sinks
    m_static_console_sink =
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    m_static_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        "logs/chart_plotter_dev.log", true);

    spdlog::set_level(config.log_level);

    std::vector<spdlog::sink_ptr> sinks{m_static_console_sink,
                                        m_static_file_sink};

    m_component_logger = std::make_shared<spdlog::async_logger>(
        "ChartPlotter", sinks.begin(), sinks.end(), spdlog::thread_pool(),
        spdlog::async_overflow_policy::overrun_oldest);

    m_component_logger->set_level(config.log_level);
    m_component_logger->set_pattern(
        "[%Y-%m-%d %H:%M:%S.%e] [ChartPlotter] [%^%l%$] [thread %t] %v");

  } catch (const spdlog::spdlog_ex &ex) {
    std::cerr << "ChartPlotter log initialization failed: " << ex.what()
              << std::endl;
  }
}

std::shared_ptr<spdlog::logger>
LoggerManager::createInstanceLogger(const std::string &instance_name) {
  try {
    // Retrieve the sinks you initialized in LoggerManager::init()
    // (You would need to store console_sink and file_sink as static variables)
    std::vector<spdlog::sink_ptr> sinks{m_static_console_sink,
                                        m_static_file_sink};

    // Create a unique logger instance that routes to the same background thread
    auto logger = std::make_shared<spdlog::async_logger>(
        instance_name, // This name will now show up in the %n pattern slot!
        sinks.begin(), sinks.end(), spdlog::thread_pool(),
        spdlog::async_overflow_policy::overrun_oldest);

    logger->set_level(spdlog::get_level());

    // [Timestamp] [Instance Name] [Log Level (colored)] [Thread ID] Message
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");

    return logger;
  } catch (const spdlog::spdlog_ex &ex) {
    std::cerr << "ChartPlotter instance logger create failed: " << ex.what()
              << std::endl;
    return spdlog::null_logger_mt(instance_name);
  }
}

std::shared_ptr<spdlog::logger> LoggerManager::get() {
  return m_component_logger;
}

void LoggerManager::shutdown() {
  if (m_component_logger) {
    m_component_logger.reset();
  }
  // Ensure all remaining messages in the queue are flushed before exit
  spdlog::shutdown();
}
