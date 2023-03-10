#include "tuntap.hpp"
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <memory.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>


TUNDevice::TUNDevice(std::string name, mode mode, int flags) {
    int err;

    if((m_fd = open("/dev/net/tun", flags)) < 0) {
        throw std::runtime_error("Can't open /dev/net/tun");
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    switch (mode) {
        case mode::TUN:
            ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
            break;
        case mode::TAP:
            ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
            break;
    }

    if(name.data())
        strncpy(ifr.ifr_name, name.c_str(), name.size());

    if((err = ioctl(m_fd, TUNSETIFF, (void *) &ifr)) < 0) {
        close(m_fd);
        throw std::runtime_error("ioctl returned " + std::to_string(err));
    }

    m_mode = mode;
    m_name = ifr.ifr_name;
};

TUNDevice::~TUNDevice() {
    if (m_fd != -1) {
        close(m_fd);
    }
}

size_t TUNDevice::read(void* buffer, size_t byte_count)
{
    ssize_t result = ::read(m_fd, buffer, byte_count);

    if (result < 0) {
        std::string error_message = "Bytes read returned negative number: ";
        error_message += strerror(errno);
        //throw std::runtime_error(error_message);
        std::cout << error_message << std::endl;
    }

    return static_cast<size_t>(result);
}

size_t TUNDevice::write(void* buffer, size_t byte_count)
{
    ssize_t result = ::write(m_fd, buffer, byte_count);

    if (result < 0) {
        std::string error_message = "Bytes written returned negative number: ";
        error_message += strerror(errno);
        //throw std::runtime_error(error_message);
        std::cout << error_message << std::endl;
    }

    return static_cast<size_t>(result);
}