#pragma once
#include <cstdint>
#include <string_view>
#include <arpa/inet.h>


namespace Chat {

    inline std::uint32_t GetPacketSize(const std::string_view message) {
        const auto size = message.size();
        return htonl(size);
    }
}