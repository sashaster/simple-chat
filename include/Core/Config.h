
#ifndef CHAT_CONFIG_H
#define CHAT_CONFIG_H

#include <string_view>
#include <vector>

#include <Core/Logging.h>

namespace Chat {

    struct Configuration {
        LogLevel logLevel;
        uint16_t port;
        std::string host;

        explicit Configuration(const std::vector<std::string_view> &args);
        static LogLevel ParseLogLevel(std::string_view level);
    };

    std::ostream& operator<<(std::ostream &out, const Configuration &config);

}


#endif
