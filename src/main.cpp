#include <ctime>
#include <iostream>
#include <iomanip>
#include <string>
#include "radio.hpp"
#include <chrono>
#include <thread>
#include "lockingqueue.hpp"
#include "tuntap.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

#define VERBOSE false
#define DEV     true
#define BASE    false

void read_from_tun(TUNDevice *device, LockingQueue<vector<uint8_t>>* send_queue);
void write_to_tun(TUNDevice *device, LockingQueue<vector<uint8_t>>* write_queue);
void radio_transmit(Radio* radio, LockingQueue<vector<uint8_t>>* send_queue);
void radio_recieve(Radio* radio, LockingQueue<vector<uint8_t>>* write_queue);
int setup_client_socket(const char *ip_address, int port);
int setup_server_socket(int port);

int main(int argc, char** argv)
{
    bool radioNumber = 1;

    uint8_t address[4][6] = {"1Node", "2Node", "3Node", "4Node"};

    cout << "Which radio is this? Enter '0' or '1'. Defaults to '0' ";
    string input;
    getline(cin, input);
    radioNumber = input.length() > 0 && (uint8_t)input[0] == 49;

    Radio radio_tx(17, address[!radioNumber + 1], address[radioNumber], VERBOSE);
    Radio radio_rx(27, address[!radioNumber], address[radioNumber + 1], VERBOSE);
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

                if (VERBOSE) {
                    std::cout << "Found start! Total length: " << total_ip_length << std::endl;
                }
            } else if(found_start) {
                ip_packet.insert(ip_packet.end(), payload.begin(), payload.end());
            }
        }

        size_t bytes_written = device->write(ip_packet.data(), ip_packet.size());

        if (VERBOSE) {
            cout << "ip_packet sent to tun0, bytes: " << dec << bytes_written << endl;
        }
    }
}

void radio_transmit(Radio* radio, LockingQueue<vector<uint8_t>>* send_queue)
{
    #if DEV == true
        int sockfd;
        #if BASE == true
            sockfd = setup_server_socket(4000);
        #else
            sockfd = setup_client_socket("192.168.131.132", 4001);
        #endif
    #endif

    while (true) {
        std::vector<uint8_t> data;

        send_queue->waitAndPop(data);
        #if DEV == true
            write(sockfd, (const void*) data.data(), data.size());
        #else
            radio->transmit(data);
        #endif
    }
}

void radio_recieve(Radio* radio, LockingQueue<vector<uint8_t>>* write_queue)
{
    #if DEV == true
        int sockfd;
        #if BASE == true
            sockfd = setup_server_socket(4001);
        #else
            sockfd = setup_client_socket("192.168.131.132", 4000);
        #endif
    #endif

    while (true) {
        #if DEV == true
            uint8_t payload[500];
            ssize_t bytes;

            bytes = read(sockfd, payload, 500);

            if(bytes > 0) {
                std::vector<uint8_t> data(payload, payload+bytes);
                write_queue->push(data);
            }
        #else
            radio->recieve(write_queue);
        #endif
    }
}

int setup_client_socket(const char *ip_address, int port)
{
    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port); //UDP port

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); //IPv4, UDP, IP

    if (inet_pton(AF_INET, ip_address, &server_address.sin_addr) <= 0) {
        cout << "did not connect: mobile, receive \n" << endl;
        return -1;
    }

    if(connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        cout << "client did not connect: " << ip_address << " " << "port: " << port << endl;
    } else {
        cout << "client connected: " << ip_address << " " << "port: " << port << endl;
    }

    return sockfd;
}

int setup_server_socket(int port)
{
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); //IPv4, UDP, IP
    if (sockfd < 0) {
        cout << "server socket failed, port: " << port << endl;
        return -1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        cout << "failed to setsockopt: " << port << endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&address, addrlen) < 0) {
        cout << "failed to bind port: " << port << endl;
        return -1;
    }

    if (listen(sockfd, 3)) {
        cout << "failed to listen port: " << port << endl;
        return -1;
    }

    int new_socket = accept(sockfd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if(new_socket < 0) {
        cout << "server did not accept, port: " << port << endl;
    } else {
        cout << "server accepted, port: " << port << endl;
    }

    return new_socket;
}
