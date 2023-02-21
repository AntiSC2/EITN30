#include "radio.hpp"
#include <iostream>
#include <iomanip>
#include <exception>
#include <algorithm>

Radio::Radio(int ce_pin, uint8_t tx_address[6], uint8_t rx_address[6], bool verbose) : m_radio(ce_pin, 0)
{
    if (!m_radio.begin()) {
        throw std::runtime_error("radio hardware not responding! ce_pin " + std::to_string(ce_pin));
    }

    m_radio.enableDynamicPayloads();
    m_radio.setPALevel(RF24_PA_LOW);
    m_radio.setDataRate(RF24_2MBPS);
    m_radio.setAutoAck(false);
    m_radio.setChannel(105);

    m_radio.openWritingPipe(tx_address);
    m_radio.openReadingPipe(1, rx_address);

    m_verbose = verbose;
}

void Radio::transmit(std::vector<uint8_t> data)
{
    m_radio.stopListening();

    if (m_verbose) {
        std::cout << "Starting transmission, data sent: " << std::endl;
    }

    size_t bytes_to_send = data.size();
    size_t offset = 0;

    while (bytes_to_send > 0) {
        bool result = m_radio.write(data.data() + offset, std::min(bytes_to_send, size_t(32)));

        if (!result) {
            std::cout << "Transmission failed or timed out!" << std::endl;
            return;
        } else if (m_verbose) {
            std::cout << std::setfill('0') << std::setw(2) << std::uppercase << std::hex;
            for (int i = 0; i < std::min(bytes_to_send, size_t(32)); i++) {
                std::cout << int(data[offset + i]);
            }
        }

        offset += std::min(bytes_to_send, size_t(32));
        bytes_to_send -= 32;
    }
}

std::vector<uint8_t> Radio::recieve()
{
    m_radio.startListening();
}
