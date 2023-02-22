#include "radio.hpp"
#include <chrono>
#include <thread>
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
    m_radio.setAutoAck(true);
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

        std::this_thread::sleep_for(std::chrono::microseconds(500));
        offset += std::min(bytes_to_send, size_t(32));
        bytes_to_send -= std::min(bytes_to_send, size_t(32));
    }

    if (m_verbose) {
        std::cout << std::endl;
    }
}

std::vector<uint8_t> Radio::recieve()
{
    m_radio.startListening();

    std::chrono::time_point<std::chrono::system_clock> start, end;
    size_t packets_received = 0;
    uint8_t payload[32];
    start = end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_s = end - start;

    while (elapsed_s.count() < 240) { // use 240 second timeout
        uint8_t pipe;
        if (m_radio.available(&pipe)) {
            uint8_t bytes = m_radio.getDynamicPayloadSize();

            if (bytes < 1) {
                continue;
            }

            m_radio.read(&payload, bytes);

            if (m_verbose) {
                std::cout << "Received " << std::dec << std::setw(0) << std::setfill(' ') << (unsigned int)bytes;
                std::cout << " bytes on pipe " << (unsigned int)pipe;
                std::cout << ": ";
                std::cout << std::setfill('0') << std::setw(2) << std::uppercase << std::hex;
                for (int i = 0; i < bytes; i++) {
                    std::cout << int(payload[i]);
                }
                std::cout << std::endl;
            }

            packets_received += 1;
            start = std::chrono::system_clock::now();
        }
        end = std::chrono::system_clock::now();
        elapsed_s = end - start;
    }

    m_radio.stopListening();
    return std::vector<uint8_t>();
}
