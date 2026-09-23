#include <Core/Config.hpp>
#include <Core/Logging.hpp>
#include <gtest/gtest.h>

using namespace Chat;

TEST(ConfigurationTest, ShouldUseProvidedArguments) {
    std::vector<std::string_view> args = {"--log-level=debug", "--port=80", "--host=localhost"};
    auto cfg = Configuration(args);
    EXPECT_EQ(cfg.logLevel, LogLevel::Debug);
    EXPECT_EQ(cfg.port, 80);
    EXPECT_EQ(cfg.host, "localhost");
}

TEST(ConfigurationTest, ShouldUseDefaultArguments) {
    auto cfg = Configuration(std::vector<std::string_view>{});
    EXPECT_EQ(cfg.logLevel, LogLevel::Info);
    EXPECT_EQ(cfg.port, 8080);
    EXPECT_EQ(cfg.host, "127.0.0.1");
}

