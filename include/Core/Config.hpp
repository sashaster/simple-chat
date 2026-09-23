#pragma once

#include <string_view>
#include <vector>

#include <Core/Logging.hpp>

namespace Chat {

    struct Configuration {
        LogLevel logLevel;
        uint16_t port;
        std::string host;

        explicit Configuration(const std::vector<std::string_view> &args);
        static LogLevel ParseLogLevel(std::string_view level);
    };

}
