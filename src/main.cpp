#include <format>
#include <vector>
#include <csignal>

#include <Core/Config.h>
#include <Core/Logging.h>
#include <Core/Server.h>
#include <Core/Client.h>


int main(const int argc, char* argv[]) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    if (args.empty()) {
        std::exit(EXIT_FAILURE);
    }
    const auto app = args[0];
    if (app != "client" && app != "server") {
        std::exit(EXIT_FAILURE);
    }
    std::signal(SIGPIPE, SIG_IGN);
    const auto cfg = Chat::Configuration(args);
    const auto logger = Chat::Logger(cfg.logLevel);
    Chat::SetDefaultLogger(logger);
    logger.Info("", cfg);
    if (app == "server") {
        std::signal(SIGINT, &Chat::Server::Stop);
        std::signal(SIGTERM, &Chat::Server::Stop);
        auto server = Chat::Server(cfg);
        server.Listen();
    }
    else {
        auto client = Chat::Client(cfg);
        std::signal(SIGINT, &Chat::Client::Disconnect);
        std::signal(SIGTERM, &Chat::Client::Disconnect);
        try {
            client.Connect();
        }catch (const std::exception& e) {
            Chat::GetLogger().Error(e.what());
        }}

    return 0;
}
