#pragma once
#include <RF24.h>
#include <string>
#include <vector>
#include "lockingqueue.hpp"
#include <iostream>
#include <iomanip>

class Radio {
public:
    Radio(int ce_pin, uint8_t tx_address[6], uint8_t rx_address[6], bool verbose=false);

    void setListening(bool listen);

    inline void transmit(std::vector<uint8_t> &data)
    {
        if (m_verbose) {
            std::cout << "Starting transmission, data sent: " << std::endl;
        }

        size_t bytes_to_send = data.size();
        size_t offset = 0;

        while (bytes_to_send > 0) {
            uint8_t payload[32];
            memcpy(payload + 1, data.data() + offset, std::min(bytes_to_send, size_t(31)));

            if (bytes_to_send < 32) {
                payload[0] = 1;
            } else {
                payload[0] = 0;
            }

            bool result = m_radio.writeFast(payload, std::min(bytes_to_send + 1, size_t(32)));

            if (!result) {
                m_radio.txStandBy();
                result = m_radio.writeFast(payload, std::min(bytes_to_send + 1, size_t(32)));

                if (m_verbose) {
                    std::cout << "Retrying write!" << std::endl;
                }

                if (!result) {
                    std::cout << "Transmission failed or timed out!" << std::endl;
                    return;
                }
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

    inline void recieve(LockingQueue<std::vector<uint8_t>>* write_queue)
    {
        uint8_t payload[32];
        uint8_t pipe;
        if (m_radio.available(&pipe)) {
            uint8_t bytes = m_radio.getPayloadSize();

            m_radio.read(&payload, bytes);

            if (m_verbose) {
                std::cout << "Received " << std::dec << std::setw(0) << std::setfill(' ') << 32;
                std::cout << " bytes on pipe " << (unsigned int)pipe;
                std::cout << ": ";
                std::cout << std::setfill('0') << std::setw(2) << std::uppercase << std::hex;
                for (int i = 0; i < bytes; i++) {
                    std::cout << int(payload[i]);
                }
                std::cout << std::endl;
            }

            std::vector<uint8_t> data(payload, payload+32);
            write_queue->push(data);
        }
    }
private:
    bool m_verbose;
    RF24 m_radio;
};
