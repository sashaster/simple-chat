#pragma once
#include <arpa/inet.h>
#include <optional>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <cstdint>
#include <expected>
#include <string_view>

#include <Core/Config.hpp>
#include <Core/Logging.hpp>


namespace Chat {

    struct ClientConnection {
        int socket;
        std::string ip;
        std::uint16_t port;

        [[nodiscard]] std::string ToString() const {
            return ip + ":" + std::to_string(port);
        }
        [[nodiscard]] std::expected<bool, std::error_code> IsReady() const;

        ClientConnection(const int socket, const std::string_view ip, const int port): socket(socket), ip(ip), port(port) {}
        ~ClientConnection();
    };


    class Server {
    private:
        sockaddr_in m_Conn;
        int m_Socket;
        inline static std::atomic<bool> s_Running = false;
        std::mutex m_Mutex;
        std::vector<std::shared_ptr<ClientConnection>> m_Connections;
        std::vector<std::jthread> m_Workers;
        void Handle(const std::shared_ptr<ClientConnection>& client);
        [[nodiscard]] std::expected<std::shared_ptr<ClientConnection>, std::error_code> Accept() const;
        [[nodiscard]] std::expected<std::string, std::error_code> Receive(const ClientConnection &client) const;
        [[nodiscard]] std::expected<std::size_t, std::error_code> Send(std::string_view message, const ClientConnection &recipient) const;
        [[nodiscard]] std::expected<std::size_t, std::error_code> Read(const ClientConnection &client, void* data, std::size_t size) const;
        void Broadcast(std::string_view message, const ClientConnection &sender);
        [[nodiscard]] std::string GetAddress() const;
        [[nodiscard]] std::expected<bool, std::error_code> IsReady() const;

    public:
        explicit Server(const Configuration &config);
        void Listen();
        static void Shutdown(int);
        ~Server();
    };

}

