#include <unistd.h>
#include <thread>
#include <cstring>
#include <cstdint>
#include <iostream>

#include <Core/Client.hpp>
#include <Core/Server.hpp>
#include <Core/Utils.hpp>

namespace Chat {

    Client::Client(const Configuration &config): m_Socket(0) {
        m_ServerConn.sin_family = AF_INET;
        m_ServerConn.sin_port = htons(config.port);
        inet_pton(AF_INET, config.host.c_str(), &m_ServerConn.sin_addr);
    }

    std::expected<bool, std::error_code> Client::IsReady() const {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(m_Socket, &rfds);

        timeval tm{.tv_sec = 1, .tv_usec = 0};
        if (const int ready = select(m_Socket + 1, &rfds, nullptr, nullptr, &tm); ready < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }
        else if (ready == 0) {
            return false;
        }
        return true;
    }

    std::expected<std::size_t, std::error_code> Client::Read(void* data, const std::size_t size) const {
        std::size_t totalRead = 0;
        auto* bytes = static_cast<std::byte*>(data);
        while (totalRead < size) {
            const auto ready = IsReady();
            if (!ready) {
                return std::unexpected(ready.error());
            }
            if (!ready.value()) {
                return std::unexpected(std::error_code(std::make_error_code(std::errc::timed_out).value(), std::system_category()));
            }

            const auto n = recv(m_Socket, bytes + totalRead, size - totalRead, 0);
            if (n < 0) {
                return std::unexpected(std::error_code(errno, std::system_category()));
            }
            if (n == 0){
                return std::unexpected(std::error_code(std::make_error_code(std::errc::not_connected).value(), std::system_category()));
            }
            totalRead += n;
        }
        return totalRead;

    }

    std::expected<std::string, std::error_code> Client::Receive() const {
        std::uint32_t packetSize = 0;
        auto res = Read(&packetSize, sizeof(packetSize));
        if (!res) {
            return std::unexpected(res.error());
        }

        const auto messageSize = ntohl(packetSize);
        std::string message(messageSize, ' ');
        res = Read(message.data(), message.size());
        if (!res) {
            return std::unexpected(res.error());
        }

        return message;
    }

    void Client::Connect() {
        m_Socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_Socket < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        if (connect(m_Socket, reinterpret_cast<sockaddr *>(&m_ServerConn), sizeof(m_ServerConn)) < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        s_Connected = true;
        m_Reader = std::jthread([this]() {
            while (s_Connected) {
                if (const auto message = Receive(); !message) {
                    if (message.error() == std::errc::timed_out) {
                        continue;
                    }
                    if (message.error() == std::errc::not_connected) {
                        break;
                    }
                    GetLogger().Error("{}", message.error().message());
                }
                else {
                    std::lock_guard lock(m_Mutex);
                    std::println("\r{}", message.value());
                    std::print(">");
                }
            }
        });

        while (s_Connected) {
                {
                    std::lock_guard lock(m_Mutex);
                    std::print("\r>");
                }
                std::string msg;
                std::getline(std::cin, msg);
                if (!msg.empty()) {
                    if (const auto res = Send(msg); !res) {
                        GetLogger().Error("{}", res.error().message());
                    }
                }
        }
    }

    void Client::Disconnect(int) {
        s_Connected = false;
    }

    std::expected<std::size_t, std::error_code> Client::Send(const std::string_view message) const {
        const auto packetSize = GetPacketSize(message);
        auto bytes = send(m_Socket, &packetSize, sizeof(packetSize), 0);
        if (bytes < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }

        bytes = send(m_Socket, message.data(), message.length(), 0);
        if (bytes < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }
        GetLogger().Debug("server <-{}", message);
        return bytes;
    }

    Client::~Client() {
        GetLogger().Info("Disconnecting from server...");
        shutdown(m_Socket, SHUT_RDWR);
        if (m_Socket >= 0) {
            close(m_Socket);
            m_Socket = -1;
            GetLogger().Info("Disconnected from server");
        }
    }
}