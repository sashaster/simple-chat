#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <arpa/inet.h>
#include <optional>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>

#include <Core/Config.h>
#include <Core/Logging.h>


namespace Chat {

    struct ClientConnection {
        int socket;
        std::string addr;

        ClientConnection(const int socket, const std::string &ip, const int port): socket{socket}, addr(ip + ":" + std::to_string(port)) {}
        ClientConnection(ClientConnection &&other) noexcept;
        ~ClientConnection();
        [[nodiscard]] std::string ReceivePrefix() const;
        [[nodiscard]] std::string SendPrefix() const;
    };

    class timeout_exception : public std::exception {
    public:
        timeout_exception() = default;
        [[nodiscard]] const char *what() const noexcept override;
    };

    class Server {
    private:
        sockaddr_in m_Conn;
        int m_Socket;
        static std::atomic<bool> s_Running;
        std::mutex m_Mutex;
        std::vector<std::shared_ptr<ClientConnection>> m_Connections;
        std::vector<std::jthread> m_Workers;
        void Handle(std::shared_ptr<ClientConnection> client);
        [[nodiscard]] ClientConnection Accept(int timeout = 1) const;
        [[nodiscard]] std::optional<std::string> Receive(const ClientConnection &client, int timeout = 1) const;
        void Send(const std::string &message, const ClientConnection &recipient, const ClientConnection &sender) const;
        void Broadcast(const std::string &message, const ClientConnection &sender);

    public:
        explicit Server(const Configuration &config);
        void Listen();
        static void Stop(int);
        ~Server();
    };

}

#endif
