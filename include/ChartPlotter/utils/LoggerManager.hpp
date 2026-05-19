#pragma once

#include "spdlog/common.h"
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

class LoggerManager {
public:
  struct Config {
    spdlog::level::level_enum log_level;
  };

  static void init(Config config);
  static void init();
  static void shutdown();

  static std::shared_ptr<spdlog::logger> get();
  static std::shared_ptr<spdlog::logger>
  createInstanceLogger(const std::string &instance_name);

private:
  static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt>
      m_static_console_sink;
  static std::shared_ptr<spdlog::sinks::basic_file_sink_mt> m_static_file_sink;
  static std::shared_ptr<spdlog::async_logger> m_component_logger;
};

#define CP_TRACE(...)                                                          \
  if (LoggerManager::get())                                                    \
  LoggerManager::get()->trace(__VA_ARGS__)
#define CP_DEBUG(...)                                                          \
  if (LoggerManager::get())                                                    \
  LoggerManager::get()->debug(__VA_ARGS__)
#define CP_INFO(...)                                                           \
  if (LoggerManager::get())                                                    \
  LoggerManager::get()->info(__VA_ARGS__)
#define CP_WARN(...)                                                           \
  if (LoggerManager::get())                                                    \
  LoggerManager::get()->warn(__VA_ARGS__)
#define CP_ERROR(...)                                                          \
  if (LoggerManager::get())                                                    \
  LoggerManager::get()->error(__VA_ARGS__)
#define CP_CRITICAL(...)                                                       \
  if (LoggerManager::get())                                                    \
  LoggerManager::get()->critical(__VA_ARGS__)
