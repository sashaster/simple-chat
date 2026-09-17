#include <unistd.h>
#include <thread>
#include <cstring>

#include <Core/Client.h>
#include <Core/Server.h>

namespace Chat {

    Client::Client(const Configuration &config): m_Socket(0) {
        m_ServerConn.sin_family = AF_INET;
        m_ServerConn.sin_port = htons(config.port);
        inet_pton(AF_INET, config.host.c_str(), &m_ServerConn.sin_addr);
    }

    std::optional<std::string> Client::Receive(const int timeout) const {
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
        std::string message(4096, ' ');
        const auto res = recv(m_Socket, message.data(), message.size(), 0);
        if (res == 0) {
            return {};
        }
        if (res < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        message.resize(res);
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
                try {
                    if (const auto message = Receive()) {
                        std::lock_guard<std::mutex> lock(m_Mutex);
                        std::cout <<'\r'<< *message << std::endl;
                        std::cout << ">" << std::flush;
                    }
                }catch (const timeout_exception &e) {
                    continue;
                }
                catch (const std::exception &e) {
                    GetLogger().Error(e.what());
                }
            }
        });

        while (s_Connected) {
            try {
                {
                    std::lock_guard lock(m_Mutex);
                    std::cout << "\r>" << std::flush;
                }
                std::string msg;
                std::getline(std::cin, msg);
                if (!msg.empty()) {
                    Send(msg);
                }
            }catch (const std::exception &e) {
                GetLogger().Error(e.what());
            }
        }
    }

    void Client::Disconnect(int) {
        s_Connected = false;
    }

    void Client::Send(const std::string_view message) const {
        if (send(m_Socket, message.data(), message.length(), 0) < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        GetLogger().Debug(std::format("server<- {}", message));
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

    std::atomic<bool> Client::s_Connected = false;

}