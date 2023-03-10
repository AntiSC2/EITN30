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

#ifndef MTU
#define MTU 500
#endif

using namespace std;

void read_from_tun(TUNDevice *device, LockingQueue<vector<uint8_t>>* send_queue);
void write_to_tun(TUNDevice *device, LockingQueue<vector<uint8_t>>* write_queue);
void radio_transmit(Radio* radio, LockingQueue<vector<uint8_t>>* send_queue);
void radio_recieve(Radio* radio, LockingQueue<vector<uint8_t>>* write_queue);
int setup_client_socket(int port);
int setup_server_socket(int port);

int main(int argc, char** argv)
{
    #ifdef BASE
    bool radioNumber = 1;
    #else
    bool radioNumber = 0;
    #endif

    cout << "MTU set to: " << MTU << endl;
    cout << "Radio number set to: " << (int)radioNumber << endl;

    uint8_t address[4][6] = {"1Node", "2Node", "3Node", "4Node"};

    #ifndef VERBOSE
    Radio radio_tx(17, address[!radioNumber + 1], address[radioNumber], false);
    Radio radio_rx(27, address[!radioNumber], address[radioNumber + 1], false);
    #else
    Radio radio_tx(17, address[!radioNumber + 1], address[radioNumber], true);
    Radio radio_rx(27, address[!radioNumber], address[radioNumber + 1], true);
    #endif

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
    unsigned char payload[MTU];

    while (true) {
        size_t bytes_read = device->read(&payload, MTU);
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
        bool found_end = false;
        std::vector<uint8_t> ip_packet;

        while(!found_end && ip_packet.size() < MTU) {
            write_queue->waitAndPop(payload);

            if(payload[0] == 1) {
                found_end = true;
                ip_packet.insert(ip_packet.end(), payload.begin() + 1, payload.end());

                #ifdef VERBOSE
                cout << "Found end! Total length: " << dec << ip_packet.size() << std::endl;
                #endif
                break;
            }

            ip_packet.insert(ip_packet.end(), payload.begin() + 1, payload.end());
        }

        if(found_end && ip_packet.size() > 0) {
            size_t bytes_written = device->write(ip_packet.data(), ip_packet.size());
            #ifdef VERBOSE
            cout << "ip_packet sent to tun0, bytes: " << dec << bytes_written << endl;
            #endif
        }
    }
}

void radio_transmit(Radio* radio, LockingQueue<vector<uint8_t>>* send_queue)
{
    #ifdef USE_UDP
    struct sockaddr_in sendto_address;
    int sockfd;

    memset(&sendto_address, 0, sizeof(sendto_address));
    sendto_address.sin_family = AF_INET;

    #ifdef BASE
        sockfd = setup_client_socket(4000);
        sendto_address.sin_port = htons(4000);
        sendto_address.sin_addr.s_addr = inet_addr("192.168.131.172");
    #else
        sockfd = setup_client_socket(4001);
        sendto_address.sin_port = htons(4001);
        sendto_address.sin_addr.s_addr = inet_addr("192.168.131.132");
    #endif
    #endif

    while (true) {
        std::vector<uint8_t> data;

        send_queue->waitAndPop(data);
        #ifdef USE_UDP
        ssize_t bytes = sendto(sockfd, (const void*) data.data(), data.size(), MSG_CONFIRM, (const struct sockaddr*) &sendto_address, sizeof(sendto_address));
        cout << "Sending data: " << bytes << endl;
        #else
        radio->transmit(data);
        #endif
    }
}

void radio_recieve(Radio* radio, LockingQueue<vector<uint8_t>>* write_queue)
{
    #ifdef USE_UDP
    struct sockaddr_in recieve_addr;
    memset(&recieve_addr, 0, sizeof(recieve_addr));
    int sockfd;
    #ifdef BASE
        sockfd = setup_server_socket(4001);
    #else
        sockfd = setup_server_socket(4000);
    #endif
    #endif

    while (true) {
        #ifdef USE_UDP
        uint8_t payload[MTU];
        ssize_t bytes;

        socklen_t len = sizeof(recieve_addr);
        bytes = recvfrom(sockfd, payload, MTU, 0, (struct sockaddr*)&recieve_addr, &len);
        cout << "Bytes recieved: " << bytes << endl;

        if(bytes > 0) {
            std::vector<uint8_t> data(payload, payload+bytes);
            write_queue->push(data);
        }
        #else
        radio->recieve(write_queue);
        #endif
    }
}

int setup_client_socket(int port)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); //IPv4, UDP, IP
    if (sockfd < 0) {
        cout << "client socket creation failed, port: " << port << endl;
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

    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&address, addrlen) < 0) {
        cout << "failed to bind port: " << port << endl;
        return -1;
    }

    if (sockfd > 0) {
        cout << "server ready! port: " << port << endl;
    }

    return sockfd;
}
