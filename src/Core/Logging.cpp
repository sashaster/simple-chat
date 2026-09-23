#include <Core/Logging.hpp>


namespace Chat {

    static auto s_Logger = Logger(LogLevel::Info);

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

