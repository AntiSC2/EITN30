#include <ctime>
#include <iostream>
#include <iomanip>
#include <string>
#include "radio.hpp"
#include <chrono>
#include <thread>
#include "lockingqueue.hpp"
#include "tuntap.hpp"

using namespace std;

bool verbose = false;

void read_from_tun(TUNDevice *device, LockingQueue<vector<uint8_t>>* send_queue);
void write_to_tun(TUNDevice *device, LockingQueue<vector<uint8_t>>* write_queue);
void radio_transmit(Radio* radio, LockingQueue<vector<uint8_t>>* send_queue);
void radio_recieve(Radio* radio, LockingQueue<vector<uint8_t>>* write_queue);

int main(int argc, char** argv)
{
    bool radioNumber = 1;

    uint8_t address[4][6] = {"1Node", "2Node", "3Node", "4Node"};

    cout << "Which radio is this? Enter '0' or '1'. Defaults to '0' ";
    string input;
    getline(cin, input);
    radioNumber = input.length() > 0 && (uint8_t)input[0] == 49;

    Radio radio_tx(17, address[!radioNumber + 1], address[radioNumber], verbose);
    Radio radio_rx(27, address[!radioNumber], address[radioNumber + 1], verbose);
    TUNDevice device("tun0", mode::TUN, 2);
    radio_tx.setListening(false);
    radio_rx.setListening(true);

    LockingQueue<vector<uint8_t>> send_queue;
    LockingQueue<vector<uint8_t>> write_queue;

    thread read_thread(read_from_tun, &device, &send_queue);
    thread write_thread(write_to_tun, &device, &write_queue);
    thread rx_thread(radio_recieve, &radio_rx, &write_queue);
    read_thread.detach();
    write_thread.detach();
    rx_thread.detach();

    radio_transmit(&radio_tx, &send_queue);

    return 0;
}

void read_from_tun(TUNDevice* device, LockingQueue<vector<uint8_t>>* send_queue)
{
    unsigned char payload[1024];

    while (true) {
        size_t bytes_read = device->read(&payload, 1024);
        payload[bytes_read] = '\0';
        vector<uint8_t> data(payload, payload + bytes_read);

        if (data.size() > 0) {
            send_queue->push(data);
        }
    }
}

void write_to_tun(TUNDevice* device, LockingQueue<vector<uint8_t>>* write_queue)
{
    while (true) {
        std::vector<uint8_t> payload;
        bool found_start = false;
        std::vector<uint8_t> ip_packet;
        uint16_t total_ip_length = 0;

        while(!found_start || ip_packet.size() < total_ip_length) {
            write_queue->waitAndPop(payload);

            if(!found_start && int(payload[0] & 0b11110000) == 64) {
                found_start = true;
                ip_packet.insert(ip_packet.end(), payload.begin(), payload.end());
                total_ip_length = ((uint16_t)ip_packet[2] << 8) + (uint16_t)ip_packet[3];

                if (verbose) {
                    std::cout << "Found start! Total length: " << total_ip_length << std::endl;
                }
            } else if(found_start) {
                ip_packet.insert(ip_packet.end(), payload.begin(), payload.end());
            }
        }

        size_t bytes_written = device->write(ip_packet.data(), ip_packet.size());

        if (verbose) {
            cout << "ip_packet sent to tun0, bytes: " << dec << bytes_written << endl;
        }
    }
}

void radio_transmit(Radio* radio, LockingQueue<vector<uint8_t>>* send_queue)
{
    while (true) {
        std::vector<uint8_t> data;

        send_queue->waitAndPop(data);

        radio->transmit(data);
    }
}

void radio_recieve(Radio* radio, LockingQueue<vector<uint8_t>>* write_queue)
{
    while (true) {
        radio->recieve(write_queue);
    }
}
