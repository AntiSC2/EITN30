#pragma once
#include <RF24.h>
#include <string>
#include <vector>
#include "lockingqueue.hpp"

class Radio {
public:
    Radio(int ce_pin, uint8_t tx_address[6], uint8_t rx_address[6], bool verbose=false);

    void setListening(bool listen);
    inline void transmit(std::vector<uint8_t> data);
    inline void recieve(LockingQueue<std::vector<uint8_t>>* write_queue);
private:
    bool m_verbose;
    RF24 m_radio;
};
