/**
 * @file        Common/ME3TweaksLogger.hpp
 * @brief       Logger wrapper that prepends boot time to all log messages
 */

#pragma once

#include <chrono>
#include <memory>
#include <spdlog/spdlog.h>

namespace Common
{

/**
 * @brief   Logger wrapper that prepends time since initialization (seconds.milliseconds) to every log message
 * @remarks This class wraps an spdlog::logger and automatically prefixes all log calls with
 *          the time elapsed since the logger was created.
 *          Format: "[SSSS.MMM] message" where SSSS is seconds and MMM is milliseconds
 */
class ME3TweaksLogger
{
public:

    enum LogOutput {
        OutputToFile = 1,
        OutputToConsole = 2
    };

    /**
     * @brief   Construct a TimestampedLogger that wraps an existing logger
     * @param   logger_name  name for the spdlog logger
     * @param   outputType  where to log, can be multiple values
     * @param   filename  file to log to, if OutputToFile is specified
     */
    explicit ME3TweaksLogger(const std::string &logger_name, const LogOutput outputType, const spdlog::filename_t &filename);

    /**
     * @brief   Get the underlying spdlog logger
     * @return  Shared pointer to the wrapped logger
     */
    std::shared_ptr<spdlog::logger> GetLogger() const { return m_logger; }

    /**
     * @brief   Get elapsed time since logger creation in seconds.milliseconds format
     * @return  String formatted as "SSSS.MMM"
     */
    std::string GetBootTimeString() const;

    // ! Logging methods with boot time prefix
    // ========================================

    template<typename... Args>
    void log(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestamp(spdlog::level::trace, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void trace(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestamp(spdlog::level::trace, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestamp(spdlog::level::debug, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestamp(spdlog::level::info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestamp(spdlog::level::warn, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestamp(spdlog::level::err, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void critical(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestamp(spdlog::level::critical, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log(spdlog::wformat_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestampW(spdlog::level::trace, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void trace(spdlog::wformat_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestampW(spdlog::level::trace, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(spdlog::wformat_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestampW(spdlog::level::debug, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(spdlog::wformat_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestampW(spdlog::level::info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(spdlog::wformat_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestampW(spdlog::level::warn, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(spdlog::wformat_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestampW(spdlog::level::err, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void critical(spdlog::wformat_string_t<Args...> fmt, Args&&... args)
    {
        LogWithTimestampW(spdlog::level::critical, fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief   Flush the underlying logger
     */
    void flush()
    {
        if (m_logger)
        {
            m_logger->flush();
        }
    }

    /**
     * @brief   Set the log level for the underlying logger
     */
    void set_level(spdlog::level::level_enum level)
    {
        if (m_logger)
        {
            m_logger->set_level(level);
        }
    }

private:
    // ! Helper functions to eliminate code duplication
    // ========================================

    template<typename... Args>
    void LogWithTimestamp(spdlog::level::level_enum level, spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if (m_logger && m_logger->should_log(level))
        {
            auto timestamp = GetBootTimeString();
            m_logger->log(level, "[{}] {}", timestamp, fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    template<typename... Args>
    void LogWithTimestampW(spdlog::level::level_enum level, spdlog::wformat_string_t<Args...> fmt, Args&&... args)
    {
        if (m_logger && m_logger->should_log(level))
        {
            auto timestamp = GetBootTimeString();
            auto timestampW = std::wstring(timestamp.begin(), timestamp.end());
            m_logger->log(level, L"[{}] {}", timestampW, std::format(fmt, std::forward<Args>(args)...));
        }
    }

    std::shared_ptr<spdlog::logger> m_logger;
    std::chrono::steady_clock::time_point m_startTime;
};

} // namespace Common
