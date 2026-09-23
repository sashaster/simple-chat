#include <iostream>

#include <Core/Config.hpp>


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
            const auto [fst, snd] = split(arg, "=");
            if (fst == "--log-level" && !snd.empty() && !log_level_set) {
                logLevel = ParseLogLevel(snd);
                log_level_set = true;
            }
            if (fst == "--port" && !snd.empty() && !port_set) {
                port = std::stoi(std::string(snd));
                port_set = true;
            }
            if (fst == "--host" && !snd.empty() && !host_set) {
                host = snd;
                host_set = true;
            }
        }
        if (!log_level_set)
            logLevel = LogLevel::Info;
        if (!port_set)
            port = 8080;
        if (!host_set)
            host = "127.0.0.1";
    }

    LogLevel Configuration::ParseLogLevel(const std::string_view level) {
        if (level == "debug")
            return LogLevel::Debug;
        if (level == "info")
            return LogLevel::Info;
        if (level == "error")
            return LogLevel::Error;
        return LogLevel::Info;
    }

}