#include <iostream>

#include <Core/Config.h>


namespace Chat {

    Configuration::Configuration(const std::vector<std::string_view> &args) {
        const auto split = [](const std::string_view arg, const std::string_view separator)-> std::pair<std::string_view, std::string_view> {
            const auto idx = arg.find(separator);
            if (idx == std::string_view::npos) {
                return {arg, {}};
            }

            return {arg.substr(0, idx), arg.substr(idx + 1)};
        };
        bool log_level_set = false;
        bool port_set = false;
        bool host_set = false;
        for (const auto arg: args) {
            const auto res = split(arg, "=");
            if (res.first == "--log-level" && !res.second.empty() && !log_level_set) {
                logLevel = ParseLogLevel(res.second);
                log_level_set = true;
            }
            if (res.first == "--port" && !res.second.empty() && !port_set) {
                port = std::stoi(std::string(res.second));
                port_set = true;
            }
            if (res.first == "--host" && !res.second.empty() && !host_set) {
                host = res.second;
                host_set = true;
            }
        }
        if (!log_level_set)
            logLevel = LogLevel::INFO;
        if (!port_set)
            port = 8080;
        if (!host_set)
            host = "127.0.0.1";
    }

    std::ostream& operator<<(std::ostream &out, const Configuration &config) {
       return out << "Configuration{log-level=" << config.logLevel <<
           " port=" << config.port << " host=" << config.host << "}";
    }

    LogLevel Configuration::ParseLogLevel(const std::string_view level) {
        if (level == "debug")
            return LogLevel::DEBUG;
        if (level == "info")
            return LogLevel::INFO;
        if (level == "error")
            return LogLevel::ERROR;
        return LogLevel::INFO;
    }

}