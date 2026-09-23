#pragma once

#include <arpa/inet.h>
#include <atomic>
#include <expected>
#include <system_error>

#include <Core/Logging.hpp>
#include <Core/Config.hpp>

namespace Chat {

    class Client {
    private:
        sockaddr_in m_ServerConn;
        int m_Socket;
        inline static std::atomic<bool> s_Connected = false;
        [[nodiscard]] std::expected<std::string, std::error_code> Receive() const;
        [[nodiscard]] std::expected<std::size_t, std::error_code> Send(std::string_view message) const;
        [[nodiscard]] std::expected<std::size_t, std::error_code> Read(void* data, std::size_t size) const;
        std::mutex m_Mutex;
        std::jthread m_Reader;
        [[nodiscard]] std::expected<bool, std::error_code> IsReady() const;

    public:
        explicit Client(const Configuration &config);
        void Connect();
        static void Disconnect(int);
        ~Client();
    };

}
