#pragma once

#include <print>
#include <chrono>
#include <mutex>
#include <format>
#include <utility>
#include <string_view>


namespace Chat {
    enum class LogLevel {
        Error,
        Info,
        Debug
    };

    inline std::string_view LogLevelToString(const LogLevel level) {
        switch (level){
            case LogLevel::Error:
                return "ERROR";
            case LogLevel::Info:
                return "INFO";
            case LogLevel::Debug:
                return "DEBUG";
            default:
                return "UNKNOWN";
        }
    }

    class Logger {
    private:
        LogLevel m_Level;
        static inline std::mutex m_Mutex;

    public:
        explicit Logger(const LogLevel level): m_Level(level) {}

        template<typename... Args>
        void Log(const LogLevel level, std::format_string<Args...> fmt, Args&&... args) const {
            if (m_Level >= level) {
                std::lock_guard lock(m_Mutex);
                const auto timestamp = std::chrono::system_clock::now();
                const auto formattedMessage = std::format(fmt, std::forward<Args>(args)...);
                std::println("{:%Y-%m-%d %H:%M:%S} [{}] {}", timestamp, LogLevelToString(level), formattedMessage);
            }
        }

        template<typename... Args>
        void Debug(std::format_string<Args...> fmt, Args&&... args) const {
            Log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void Info(std::format_string<Args...> fmt, Args&&... args) const {
            Log(LogLevel::Info, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void Error(std::format_string<Args...> fmt, Args&&... args) const {
            Log(LogLevel::Error, fmt, std::forward<Args>(args)...);
        }
    };

    Logger& GetLogger();
    void SetDefaultLogger(const Logger &logger);

}

