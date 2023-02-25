#pragma once
#include <string>

enum class mode {
    TUN, TAP
};

class TUNDevice {
public:
    TUNDevice(std::string name, mode mode, int flags);
    ~TUNDevice();

    size_t read(void* buffer, size_t byte_count);
    size_t write(void* buffer, size_t byte_count);
private:
    mode m_mode;
    std::string m_name;
    int m_fd = 0;
};