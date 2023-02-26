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
    void transmit(std::vector<uint8_t> data);
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
