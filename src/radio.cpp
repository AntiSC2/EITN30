#include "radio.hpp"
#include <exception>
#include <algorithm>

Radio::Radio(int ce_pin, uint8_t tx_address[6], uint8_t rx_address[6], bool verbose)
{
    m_radio = radio(ce_pin, 0);
    if (!radio.begin()) {
        throw std::runtime_error("radio hardware not responding! ce_pin " + std::to_string(ce_pin));
    }

    radio.enableDynamicPayloads();
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_2MBPS);
    radio.setAutoAck(false);
    radio.setChannel(105);

    radio.openWritingPipe(tx_address);
    radio.openReadingPipe(1, rx_address);

    m_verbose = verbose;
}

void transmit(std::vector<uint8_t> data)
{
    radio.stopListening();

    if (m_verbose) {
        std::cout << "Starting transmission, data sent: " << std::endl;
    }

    size_t bytes_to_send = data.size();
    size_t offset = 0;

    while (bytes_to_send > 0) {
        bool result = radio.write(data.data() + offset, std::min(bytes_to_send, 32));

        if (!result) {
            std::cout << "Transmission failed or timed out!" << std::endl;
            return;
        } else if (m_verbose) {
            std::cout << std::setfill('0') << std::setw(2) << std::uppercase << std::hex;
            for (int i = 0; i < std::min(bytes_to_send, 32); i++) {
                std::cout << int(data[offset + i]);
            }
        }

        offset += std::max(bytes_to_send, 32);
        bytes_to_send -= 32;
    }
}

td::vector<uint8_t> recieve()
{
    radio.startListening();

}