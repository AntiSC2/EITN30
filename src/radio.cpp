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
    std::chrono::time_point<std::chrono::system_clock> start, end;
    uint8_t payload[32];
    bool found_start = false;
    std::vector<uint8_t> ip_packet;
    uint16_t total_ip_length = 0;

    start = end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_s = end - start;

    while (elapsed_s.count() < 10) { // use 10 second timeout
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

            if (!found_start && (payload[2] != 8 || payload[3] != 0)) {
                continue;
            } else if (!found_start) {
                found_start = true;
                ip_packet.insert(ip_packet.end(), payload, payload + bytes);
                total_ip_length = ((uint16_t)ip_packet[6] << 8) + (uint16_t)ip_packet[7];

                if (m_verbose) {
                    std::cout << "Found start! Total length: " << total_ip_length << std::endl;
                }
            } else if (found_start) {
                ip_packet.insert(ip_packet.end(), payload, payload + bytes);

                if (ip_packet.size() > total_ip_length + 4) {
                    std::cout << "Error, payload length exceeded specified length in header" << std::endl;
                    return std::vector<uint8_t>();
                } else if (ip_packet.size() == total_ip_length + 4) {
                    return ip_packet;
                }
            }

            start = std::chrono::system_clock::now();
        }
        end = std::chrono::system_clock::now();
        elapsed_s = end - start;
    }

    return std::vector<uint8_t>();;
}
