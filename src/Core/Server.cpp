#include <unistd.h>
#include <sys/socket.h>
#include <thread>
#include <cstring>

#include <Core/Server.h>
#include <Core/Logging.h>


namespace Chat{

    ClientConnection::~ClientConnection() {
        if (socket >= 0) {
            shutdown(socket, SHUT_RDWR);
            close(socket);
            GetLogger().Info(std::format("{} disconnected", addr));
        }
    }

    std::string ClientConnection::ReceivePrefix() const {
        return addr + "<- ";
    }

    std::string ClientConnection::SendPrefix() const {
        return addr + "-> ";
    }

    ClientConnection::ClientConnection(ClientConnection &&other) noexcept: socket(other.socket), addr(std::move(other.addr)) {
        other.socket = -1;
    }

    const char* timeout_exception::what() const noexcept{
        return "Timeout";
    }

    std::atomic<bool> Server::s_Running = false;

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
        if (bind(m_Socket, reinterpret_cast<sockaddr *>(&m_Conn), sizeof(m_Conn)) < 0)
            throw std::runtime_error(std::strerror(errno));
        listen(m_Socket, 5);
        s_Running = true;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &m_Conn.sin_addr, ip, INET_ADDRSTRLEN);
        int port = ntohs(m_Conn.sin_port);
        GetLogger().Info(std::format("Server listening on {}:{}", ip, port));
        while (s_Running) {
            try {
                auto client_conn = std::make_shared<ClientConnection>(Accept(1));
                {
                    m_Connections.push_back(client_conn);
                }
                GetLogger().Info(std::format("Client connected: {}", client_conn->addr));
                m_Workers.emplace_back(&Server::Handle, this, client_conn);
            }
            catch (const timeout_exception &e) {
                continue;
            }
            catch (const std::exception &e) {
                GetLogger().Error(e.what());
            }
        }
    }

    void Server::Broadcast(const std::string &message, const ClientConnection &sender){
        std::vector<std::shared_ptr<ClientConnection>> connections;
        {
            std::lock_guard lock(m_Mutex);
            connections = m_Connections;
        }
        for (const auto &recipient : connections) {
            if (recipient->addr == sender.addr) {
                continue;
            }
            Send(message, *recipient, sender);
        }
    }

    void Server::Send(const std::string &message, const ClientConnection &recipient, const ClientConnection &sender) const {
        const auto msg = sender.SendPrefix() + message;
        if (send(recipient.socket, msg.data(), msg.length(), 0) < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        GetLogger().Debug(recipient.ReceivePrefix() + message);
    }

    void Server::Stop(int) {
        s_Running = false;
    }

    void Server::Handle(const std::shared_ptr<ClientConnection> client){
        while (s_Running) {
            try {
                const auto message = Receive(*client, 1);
                if (!message) {
                    break;
                }
                GetLogger().Debug(std::format("{}-> {}", client->addr, *message));
                Broadcast(*message, *client);

            }catch (const timeout_exception &e) {
                continue;
            }
            catch (const std::exception &e) {
                GetLogger().Error(e.what());
            }
        }
        {
            std::lock_guard lock(m_Mutex);
            std::erase(m_Connections, client);
        }
    }

    std::optional<std::string> Server::Receive(const ClientConnection &client, const int timeout) const{
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(client.socket, &rfds);

        timeval tm{.tv_sec = timeout, .tv_usec = 0};
        const int ready = select(client.socket + 1, &rfds, nullptr, nullptr, &tm);

        if (ready < 0) {
            if (errno == EINTR)
                throw timeout_exception();
            throw std::runtime_error(std::strerror(errno));
        }
        if (ready == 0) {
            throw timeout_exception();
        }
        std::string message(4096, ' ');
        const int res = recv(client.socket, message.data(), message.size(), 0);
        if (res == 0) {
            return {};
        }
        if (res < 0) {
            throw std::runtime_error(strerror(errno));
        }
        message.resize(res);

        return message;
    }

    ClientConnection Server::Accept(const int timeout) const{
        if (timeout < 0) {
            throw std::runtime_error("Timeout must be non-negative");
        }
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(m_Socket, &rfds);

        timeval tm{.tv_sec = timeout, .tv_usec = 0};
        const int ready = select(m_Socket + 1, &rfds, nullptr, nullptr, &tm);

        if (ready < 0) {
            if (errno == EINTR)
                throw timeout_exception();
            throw std::runtime_error(std::strerror(errno));
        }
        if (ready == 0) {
            throw timeout_exception();
        }
        sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);
        const int client_socket = accept(m_Socket, reinterpret_cast<sockaddr*>(&client_address), &client_address_length);
        if (client_socket < 0)
            throw std::runtime_error(std::strerror(errno));
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_address.sin_addr, ip, INET_ADDRSTRLEN);
        const int port = ntohs(client_address.sin_port);
        return {client_socket, ip, port};
    }

    Server::~Server() {
        GetLogger().Info("Stopping server...");
        if (m_Socket >= 0) {
            close(m_Socket);
            m_Socket = -1;
        }
        GetLogger().Info("Server stopped");
    }

}