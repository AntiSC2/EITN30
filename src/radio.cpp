#include "radio.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <iomanip>
#include <exception>
#include <algorithm>

Radio::Radio(int ce_pin, uint8_t tx_address[6], uint8_t rx_address[6], bool verbose) : m_radio()
{
    int csn_pin = (ce_pin == 27) * 10;

    if (!m_radio.begin(ce_pin, csn_pin)) {
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

void Radio::setListening(bool listen)
{
    if (listen) {
        m_radio.startListening();
    } else {
        m_radio.stopListening();
    }
}

void Radio::transmit(std::vector<uint8_t> data)
{
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
        bytes_to_send -= std::min(bytes_to_send, size_t(32));
    }

    if (m_verbose) {
        std::cout << std::endl;
    }
}

std::vector<uint8_t> Radio::recieve()
{
    uint8_t payload[32];
    bool found_start = false;
    std::vector<uint8_t> ip_packet;
    ip_packet.reserve(258);
    uint16_t total_ip_length = 0;

    while (true) {
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

            if (!found_start && bytes < 5) {
                continue;
            }

            if (!found_start && (int(payload[0] & 0b11110000) == 64)) {
                found_start = true;
                ip_packet.insert(ip_packet.end(), payload, payload + bytes);
                total_ip_length = ((uint16_t)ip_packet[3] << 8) + (uint16_t)ip_packet[4];

                if (m_verbose) {
                    std::cout << "Found start! Total length: " << total_ip_length << std::endl;
                }
            } else if (found_start) {
                ip_packet.insert(ip_packet.end(), payload, payload + bytes);

                if (ip_packet.size() > total_ip_length) {
                    std::cout << "Error, payload length exceeded specified length in header" << std::endl;
                    return std::vector<uint8_t>();
                } else if (ip_packet.size() == total_ip_length) {
                    return ip_packet;
                }
            } else {
                continue;
            }
        }
    }

    return std::vector<uint8_t>();;
}
