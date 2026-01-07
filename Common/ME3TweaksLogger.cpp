#include "Common/ME3TweaksLogger.hpp"
#include <format>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>


namespace Common
{

ME3TweaksLogger::ME3TweaksLogger(const std::string &logger_name, const LogOutput outputType, const spdlog::filename_t &filename)
    : m_startTime(std::chrono::steady_clock::now())
{
    m_logger = std::make_shared<spdlog::logger>(logger_name);

    if (outputType & LogOutput::OutputToConsole)
    {
        m_logger->sinks().push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }

    if (outputType & LogOutput::OutputToFile)
    {
        m_logger->sinks().push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true));
    }

	m_logger->set_level(spdlog::level::trace);
	m_logger->set_pattern("%v");

    m_logger->trace("[{} log initialized]", logger_name);
}

std::string ME3TweaksLogger::GetBootTimeString() const
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime);

    auto total_ms = elapsed.count();
    auto seconds = total_ms / 1000;
    auto milliseconds = total_ms % 1000;

    return std::format("{:04}.{:03}", seconds, milliseconds);
}

} // namespace Common
