#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H

#include <arpa/inet.h>
#include <atomic>

#include <Core/Logging.h>
#include <Core/Config.h>

namespace Chat {

    class Client {
    private:
        sockaddr_in m_ServerConn;
        int m_Socket;
        static std::atomic<bool> s_Connected;
        [[nodiscard]] std::optional<std::string> Receive(int timeout = 1) const;
        void Send(std::string_view message) const;
        std::mutex m_Mutex;
        std::jthread m_Reader;

    public:
        explicit Client(const Configuration &config);
        void Connect();
        static void Disconnect(int);
        ~Client();
    };

}

#endif
