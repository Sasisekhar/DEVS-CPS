#include "Callback.h"
#include "DigitalIn.h"
#include "NetworkInterface.h"
#include "PinNames.h"
#include "SocketAddress.h"
#include "TCPSocket.h"
#include "ThisThread.h"
#include "mbed.h"
#include "mbed_assert.h"
#include "thread_network_data_lib.h"
#include "us_ticker_api.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace network{
    NetworkInterface *interface = NetworkInterface::get_default_instance();
    TCPSocket socket;
    SocketAddress address;
}

namespace dbug {
    nsapi_size_or_error_t result;
}

struct Message {
    char topic[64];
    char payload[64];
} global;


void networkInfo() {
    SocketAddress a;
    network::interface->get_ip_address(&a);
    printf("IP address: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
    printf("MAc Address: %s\r\n", network::interface->get_mac_address() ? network::interface->get_mac_address() : "None");
    network::interface->get_netmask(&a);
    printf("Netmask: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
    network::interface->get_gateway(&a);
    printf("Gateway: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
}

void send_request() {
    
    uint8_t buffer[] = {0x10, 0x12, 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x02, 0x00, 0x3C, 0x00, 0x06, 'A', 'R', 'S', 'L', 'A', 'B'};
    nsapi_size_t bytes_to_send = sizeof(buffer);
    // printf("Size of packet is: %d\n", bytes_to_send);
    nsapi_size_or_error_t bytes_sent = 0;

    bytes_sent = network::socket.send(buffer, bytes_to_send);
    // printf("sent %d bytes\r\n", bytes_sent);
    printf("Connected\r\n");

}

void resolve_hostname(SocketAddress &address) {
    const char hostname[] = "broker.hivemq.com";

    /* get the host address */
    printf("\nResolve hostname %s\r\n", hostname);
    nsapi_size_or_error_t result = network::interface->gethostbyname(hostname, &address);
    if (result != 0) {
        printf("Error! gethostbyname(%s) returned: %d\r\n", hostname, result);
    }

    printf("%s address is %s\r\n", hostname, (address.get_ip_address() ? address.get_ip_address() : "None") );
}

void receive_response() {
    uint8_t buffer[100];

    dbug::result = network::socket.recv(buffer, 100);
    if (dbug::result < 0) {
        if(dbug::result == -3001) {
            return;
        } else {
            printf("Error! _socket.recv() returned: %d\n", dbug::result);
            return;
        }
        
    }

    if(buffer[0] == 0x30) { //The received packet is a publish packet (Assuming QoS is 0)

        //Deconstructing the header
        uint8_t msgLen = buffer[1];
        uint16_t topicLen = (buffer[2] << 8) | buffer[3];
        uint16_t payloadLen = (buffer[4 + topicLen] << 8) | buffer[4 + topicLen + 1];
        uint8_t topicHead = 4;
        uint8_t payloadHead = topicHead + topicLen + 2;

        char tempBuff[64];
        int index = 0;

        for(int i = topicHead; i < (topicHead + topicLen); i++) {
            tempBuff[index++] = (char) buffer[i];
        }
        tempBuff[index] = '\0';
        strcpy(global.topic,  (const char*) tempBuff);

        index = 0;
        for(int i = payloadHead; i < (payloadHead + payloadLen); i++) {
            tempBuff[index++] = buffer[i];
        }
        tempBuff[index] = '\0';
        strcpy(global.payload,  (const char*) tempBuff);

        printf("Message: %s received on topic: %s\n", global.payload, global.topic);
    } else {
        printf("received %d bytes: ", dbug::result);
        for(int i = 0; i < dbug::result; i++) {
            printf("0x%02X, ", (unsigned int) buffer[i]);
        }
        printf("\n");
    }
}

void publish() {
    
    uint8_t buffer[] = {0x30, 0x12, 0x00, 0x04, 'T', 'E', 'S', 'T', 0x00, 0x0A, 'S', 'A', 'S', 'I', 'S', 'E', 'K', 'H', 'A', 'R'};
    nsapi_size_t bytes_to_send = sizeof(buffer);
    // printf("Size of packet is: %d\n", bytes_to_send);
    nsapi_size_or_error_t bytes_sent = 0;

    bytes_sent = network::socket.send(buffer, bytes_to_send);
    // printf("sent %d bytes\n", bytes_sent);
    printf("Published\n");

}

void subscribe() {
    
    uint8_t buffer[] = {0x82, 0x09, 0x00, 0xFF, 0x00, 0x04, 'T', 'E', 'S', 'T', 0x00};
    nsapi_size_t bytes_to_send = sizeof(buffer);
    // printf("Size of packet is: %d\n", sizeof(buffer));
    nsapi_size_or_error_t bytes_sent = 0;

    bytes_sent = network::socket.send(buffer, sizeof(buffer));
    printf("sent %d bytes\n", bytes_sent);
    printf("Subscribed\n");

}

void disconnect() {
    
    uint8_t buffer[] = {0xE0, 0x00};
    nsapi_size_t bytes_to_send = sizeof(buffer);
    // printf("Size of packet is: %d\n", bytes_to_send);
    nsapi_size_or_error_t bytes_sent = 0;

    bytes_sent = network::socket.send(buffer, bytes_to_send);
    // printf("sent %d bytes\n", bytes_sent);
    printf("Disconnected\n");

}

namespace arduino{
    uint64_t millis() {
        return (us_ticker_read()/1000);
    }
}

void ping() {
    uint8_t buffer[] = {0xC0, 0x00};
    nsapi_size_t bytes_to_send = sizeof(buffer);
    //printf("Size of packet is: %d\n", bytes_to_send);
    nsapi_size_or_error_t bytes_sent = 0;

    bytes_sent = network::socket.send(buffer, bytes_to_send);
    //printf("sent %d bytes\n", bytes_sent);
    printf("Ping\n");
    //receive_response();
}


// main() runs in its own thread in the OS
int main() {

    DigitalIn button(BUTTON1);   

    printf("Connecting to the network...\r\n");
    network::interface->connect();

    networkInfo();

    network::socket.open(network::interface);
    //resolve_hostname(network::address);
    network::address.set_ip_address("134.117.52.253\0");
    network::address.set_port(1883);
    network::socket.connect(network::address);
    
    send_request();
    receive_response();

    subscribe();

    printf("Entering loop\n");

    uint64_t startTime = 0;

    network::socket.set_blocking(false);

    while (true) {

        if(arduino::millis() -  startTime > 5000) {
            publish();
            // ping();
            startTime = arduino::millis();
        }

        receive_response();

        if(button) {
            break;
        }
    }
    

    disconnect();
    printf("End\n");
    network::socket.close();
    network::interface->disconnect();


    return 0;
}