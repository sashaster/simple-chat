#include <unistd.h>
#include <sys/socket.h>
#include <thread>
#include <cstring>
#include <cstdint>

#include <Core/Server.hpp>
#include <Core/Logging.hpp>
#include <Core/Utils.hpp>


namespace Chat{

    ClientConnection::~ClientConnection() {
        if (socket >= 0) {
            shutdown(socket, SHUT_RDWR);
            close(socket);
            GetLogger().Info("{} disconnected", ToString());
        }
    }

    std::expected<bool, std::error_code> ClientConnection::IsReady() const {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(socket, &rfds);

        timeval tm{.tv_sec = 1, .tv_usec = 0};
        if (const int ready = select(socket + 1, &rfds, nullptr, nullptr, &tm); ready < 0)
            return std::unexpected(std::error_code(errno, std::system_category()));
        else if (ready == 0)
            return false;
        return true;
    }

    [[nodiscard]] std::string Server::GetAddress() const {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &m_Conn.sin_addr, ip, INET_ADDRSTRLEN);
        std::uint16_t port = ntohs(m_Conn.sin_port);
        return std::format("{}:{}", ip, port);
    }

    Server::Server(const Configuration &config): m_Socket(-1) {
        m_Conn.sin_family = AF_INET;
        m_Conn.sin_port = htons(config.port);
        inet_pton(AF_INET, config.host.c_str(), &m_Conn.sin_addr);
    }

    void Server::Listen() {
        GetLogger().Info("Starting server...");
        m_Socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_Socket < 0)
            throw std::runtime_error(std::strerror(errno));
        if (bind(m_Socket, reinterpret_cast<sockaddr*>(&m_Conn), sizeof(m_Conn)) < 0)
            throw std::runtime_error(std::strerror(errno));
        listen(m_Socket, 5);
        s_Running = true;
        GetLogger().Info("Server listening on {}", GetAddress());
        while (s_Running) {
                if (auto client_conn = Accept(); !client_conn) {
                    if (client_conn.error() == std::errc::timed_out) {
                        continue;
                    }
                    GetLogger().Error("{}", client_conn.error().message());
                } else{
                    const auto& client = client_conn.value();
                m_Connections.push_back(client);
                GetLogger().Info("Client connected: {}", client->ToString());
                m_Workers.emplace_back(&Server::Handle, this, client);
            }
        }
    }

    void Server::Broadcast(const std::string_view message, const ClientConnection &sender){
        std::vector<std::shared_ptr<ClientConnection>> connections;
        {
            std::lock_guard lock(m_Mutex);
            connections = m_Connections;
        }
        for (const auto &recipient : connections) {
            if (recipient->ToString() == sender.ToString()) {
                continue;
            }
            const auto packet = std::format("{}-> {}", sender.ToString(), message);
            if (const auto res = Send(packet , *recipient); !res) {
                GetLogger().Error("{}", res.error().message());
            }
            else {
                GetLogger().Debug("{} <-{}", recipient->ToString(), message);
            }
        }
    }

    std::expected<std::size_t, std::error_code> Server::Send(const std::string_view message, const ClientConnection &recipient) const {
        const auto packetSize = GetPacketSize(message);

        auto bytes = send(recipient.socket, &packetSize, sizeof(packetSize), 0);
        if (bytes < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }

        bytes = send(recipient.socket, message.data(), message.length(), 0);
        if (bytes < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }
        return bytes;
    }

    void Server::Shutdown(int) {
        s_Running = false;
    }

    void Server::Handle(const std::shared_ptr<ClientConnection>& client){
        while (s_Running) {
                if (const auto message = Receive(*client); !message) {
                    if (message.error() == std::errc::timed_out) {
                        continue;
                    }
                    if (message.error() == std::errc::not_connected) {
                        break;
                    }
                    GetLogger().Error("{}", message.error().message());
                }
                else {
                    GetLogger().Debug("{}-> {}", client->ToString(), message.value());
                    Broadcast(message.value(), *client);
                }
        }
        {
            std::lock_guard lock(m_Mutex);
            std::erase(m_Connections, client);
        }
    }

    std::expected<bool, std::error_code> Server::IsReady() const {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(m_Socket, &rfds);

        timeval tm{.tv_sec = 1, .tv_usec = 0};
        if (const int ready = select(m_Socket + 1, &rfds, nullptr, nullptr, &tm); ready < 0)
            return std::unexpected(std::error_code(errno, std::system_category()));
        else if (ready == 0)
            return false;
        return true;
    }

    std::expected<std::size_t, std::error_code> Server::Read(const ClientConnection &client, void* data, const std::size_t size) const {
        std::size_t totalRead = 0;
        auto* bytes = static_cast<std::byte*>(data);
        while (totalRead < size) {
            const auto ready = client.IsReady();
            if (!ready) {
                return std::unexpected(ready.error());
            }
            if (!ready.value()) {
                return std::unexpected(std::error_code(std::make_error_code(std::errc::timed_out).value(), std::system_category()));
            }

            const auto n = recv(client.socket, bytes + totalRead, size - totalRead, 0);
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

     std::expected<std::string, std::error_code> Server::Receive(const ClientConnection &client) const{
        std::uint32_t packetSize = 0;
        auto res = Read(client, &packetSize, sizeof(packetSize));
        if (!res) {
            return std::unexpected(res.error());
        }

        const auto messageSize = ntohl(packetSize);
        std::string message(messageSize, ' ');
        res = Read(client, message.data(), message.size());
        if (!res) {
            return std::unexpected(res.error());
        }
        return message;
    }

    std::expected<std::shared_ptr<ClientConnection>, std::error_code> Server::Accept() const{
        if (const auto res = IsReady(); !res)
            return std::unexpected(res.error());
        else if (!res.value())
            return std::unexpected(std::error_code(std::make_error_code(std::errc::timed_out).value(), std::system_category()));

        sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        const int clientSocket = accept(m_Socket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressLength);
        if (clientSocket < 0)
            return std::unexpected(std::error_code(errno, std::system_category()));
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddress.sin_addr, ip, INET_ADDRSTRLEN);
        const std::uint16_t port = ntohs(clientAddress.sin_port);
        return std::make_shared<ClientConnection>(clientSocket, ip, port);
    }

    Server::~Server() {
        GetLogger().Info("Stopping server...");
        if (m_Socket >= 0) {
            close(m_Socket);
        }
        GetLogger().Info("Server stopped");
    }

}