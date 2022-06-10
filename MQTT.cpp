#include "MQTT.h"
#include "Callback.h"
#include "PinNames.h"
#include "ThisThread.h"
#include "mbed.h"
#include "mbed_assert.h"
#include "thread_network_data_lib.h"
#include "us_ticker_api.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>

MQTTclient::MQTTclient(NetworkInterface *interface, SocketAddress address) {
    this-> _interface = interface;
    this->_address =  address;
}

bool MQTTclient::MQTTinit() {

    _result = this->_socket.open(this->_interface);
    if(_result < 0) {
        printf("Socket didn't open. Error: %d\n", _result);
        return false;
    }

    _result = this->_socket.connect(_address);
    if(_result < 0) {
        printf("Socket didn't connect. Error: %d\n", _result);
        return false;
    }


    return true;
}

bool MQTTclient::connect(const char* clientID) {
    return connect(NULL, NULL, clientID);
}

bool MQTTclient::connect(const char* username, const char* password, const char* clientID) { //implement usrname and password
    
    //uint8_t buffer[] = {0x10, 0x12, 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x02, 0x00, 0x3C, 0x00, 0x06, 'A', 'R', 'S', 'L', 'A', 'B'};
    

    uint8_t variable[10] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x02, 0x00, 0x3C};

    uint8_t payload[128];
    payload[0] = (uint8_t) (((uint16_t) strlen(clientID) & 0xFF00) >> 8);
    payload[1] = (uint8_t) ((uint16_t) strlen(clientID) & 0x00FF);
    //, 
    for(int i = 0; i < strlen(clientID); i++) {
        payload[i + 2] = clientID[i];
    }

    //Fixed header

    uint8_t fixed[2] = {(uint8_t)MQTTCONNECT, (uint8_t) (sizeof(variable) + strlen(clientID) + 2)};

    nsapi_size_t bytes_to_send = sizeof(variable) + strlen(clientID) + 4;
    // printf("Size of packet is: %d\n", index);

    int index = 0;
    uint8_t *buffer;

    for(int i = 0; i < bytes_to_send; i++) {
        if(i < 2) {
            buffer[i] = fixed[index++];
        } else if(i >= 2 && i < 12) {
            if(i == 2) {
                index = 0;
            }
            buffer[i] = variable[index++];
        } else if(i >= 12) {
            if(i == 12) {
                index = 0;
            }

            buffer[i] = payload[index++];
        }
    }
    
    _result = _socket.send(buffer, bytes_to_send);
    if(_result < 0) {
        printf("Send failed! Error: %d", _result);
    } else {
        // printf("sent %d bytes\r\n", _result);
        printf("Connected\r\n");
    }
    return true;

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