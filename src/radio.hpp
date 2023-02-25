#pragma once
#include <RF24.h>
#include <string>
#include <vector>

class Radio {
public:
    Radio(int ce_pin, uint8_t tx_address[6], uint8_t rx_address[6], bool verbose=false);

    void setListening(bool listen);
    void transmit(std::vector<uint8_t> data);
    std::vector<uint8_t> recieve();
private:
    bool m_verbose;
    RF24 m_radio;
};
