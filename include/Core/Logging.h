#ifndef CHAT_LOGGING_H
#define CHAT_LOGGING_H

#include <iostream>
#include <chrono>
#include <mutex>
#include <format>

namespace Chat {
    enum class LogLevel {
        ERROR,
        INFO,
        DEBUG
    };

    std::ostream& operator<<(std::ostream &out, LogLevel level);

    class Logger {
    private:
        LogLevel m_Level;
        static inline std::mutex m_Mutex;

    public:
        explicit Logger(const LogLevel level): m_Level(level) {}

        template<typename... Args>
        void Log(const LogLevel level, const std::string_view message, const Args &... args) const {
            if (m_Level >= level) {
                std::lock_guard lock(m_Mutex);
                const auto timestamp = std::chrono::system_clock::now();
                auto &out = (level == LogLevel::ERROR) ? std::cerr : std::cout;
                out << std::format("{:%Y-%m-%d %H:%M:%S}", timestamp) << " [" << level << "] " << message << " ";
                ((out << args), ...);
                out << std::endl;
            }
        }

        template<typename... Args>
        void Debug(const std::string_view message, const Args &... args) const {
            Log(LogLevel::DEBUG, message, args...);
        }

        template<typename... Args>
        void Info(const std::string_view message, const Args &... args) const {
            Log(LogLevel::INFO, message, args...);
        }

        template<typename... Args>
        void Error(const std::string_view message, const Args &... args) const {
            Log(LogLevel::ERROR, message, args...);
        }
    };

    Logger& GetLogger();
    void SetDefaultLogger(const Logger &logger);

}


#endif
