#include <Core/Logging.h>


namespace Chat {

    std::ostream& operator<< (std::ostream &out, const LogLevel level) {
        switch (level) {
            case LogLevel::INFO:
                return out << "INFO";
            case LogLevel::DEBUG:
                return out << "DEBUG";
            case LogLevel::ERROR:
                return out << "ERROR";
           default:
                return out << "INFO";
        }
    }

    static auto s_Logger = Logger(LogLevel::INFO);

    void SetDefaultLogger(const Logger &logger) {
        static auto logger_set = false;
        if (!logger_set) {
            s_Logger = logger;
            logger_set = true;
        }
    }

    Logger& GetLogger() {
        return s_Logger;
    }
}

