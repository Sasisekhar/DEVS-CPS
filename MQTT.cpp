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

    _result = _socket.connect(_address);
    if(_result < 0) {
        printf("Socket didn't connect. Error: %d\n", _result);
        return false;
    }

    this->_socket.set_blocking(false);

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

    _result = -3001;

    while(_result < 0){
        _result = this->_socket.recv(buffer, 100);
        if (_result < 0) {
            if(_result == -3001) {
                //return false;
            } else {
                printf("Error! _socket.recv() returned: %d\n", _result);
                return false;
            }
            
        }
    }

    printf("Connection sucessful");
    return true;
}

void MQTTclient::receive_response() {
    uint8_t buffer[100];

    _result = this->_socket.recv(buffer, 100);
    if (_result < 0) {
        if(_result == -3001) {
            return;
        } else {
            printf("Error! _socket.recv() returned: %d\n", _result);
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
        strcpy(this->_global._topic,  (const char*) tempBuff);

        index = 0;
        for(int i = payloadHead; i < (payloadHead + payloadLen); i++) {
            tempBuff[index++] = buffer[i];
        }
        tempBuff[index] = '\0';
        strcpy(this->_global._payload,  (const char*) tempBuff);

        printf("Message: %s received on topic: %s\n", this->_global._payload, this->_global._topic);
    } else {
        printf("received %d bytes: ", _result);
        for(int i = 0; i < _result; i++) {
            printf("0x%02X, ", (unsigned int) buffer[i]);
        }
        printf("\n");
    }
}

bool MQTTclient::publish(const char* topic, const char* message) {
    
    //uint8_t buffer[] = {0x30, 0x12, 0x00, 0x04, 'T', 'E', 'S', 'T', 0x00, 0x0A, 'S', 'A', 'S', 'I', 'S', 'E', 'K', 'H', 'A', 'R'};

    uint8_t variable[128];

    uint8_t index = 0;

    variable[index++] = (uint8_t) (((uint16_t) strlen(topic) & 0xFF00) >> 8);
    variable[index++] = (uint8_t) ((uint16_t) strlen(topic) & 0x00FF);

    for(int i = 0; i < strlen(topic); i++) {
        variable[index++] = topic[i];
    }

    variable[index++] = (uint8_t) (((uint16_t) strlen(message) & 0xFF00) >> 8);
    variable[index++] = (uint8_t) ((uint16_t) strlen(message) & 0x00FF);
    
    for(int i = 0; i < strlen(message); i++) {
        variable[index++] = message[i];
    }

    uint8_t fixed[] = {MQTTPUBLISH, index};

    uint8_t buffer[128];

    for(int i = 0; i < index + 2; i++) {
        if(i < 2) {
            buffer[i] = fixed[i];
        } else {
            buffer[i] = variable[i - 2];
        }
    }

    nsapi_size_t bytes_to_send = index + 2;
    // printf("Size of packet is: %d\n", bytes_to_send);

    _result = _socket.send(buffer, bytes_to_send);

    if(_result < 0) {
        printf("Publish failed! Error: %d", _result);
        return false;
    } else {
        // printf("sent %d bytes\r\n", _result);
        printf("Published\n");
    }

    return true;
}

bool MQTTclient::subscribe(const char* topic) {
    
    // uint8_t buffer[] = {0x82, 0x09, 0x00, 0xFF, 0x00, 0x04, 'T', 'E', 'S', 'T', 0x00};

    uint8_t payload[128];
    uint8_t index = 0;

    payload[index++] = (uint8_t) (((uint16_t) strlen(topic) & 0xFF00) >> 8);
    payload[index++] = (uint8_t) ((uint16_t) strlen(topic) & 0x00FF);

    for(int i = 0; i < strlen(topic); i++) {
        payload[index++] = topic[i];
    }

    payload[index++] = 0x00; //QoS

    uint8_t fixed[] = {MQTTSUBSCRIBE, (uint8_t)(index + 2), 0x00, 0xFF};

    uint8_t buffer[128];

    for(int i = 0; i < (index + 4); i++) {
        if(i < 4) {
            buffer[i] = fixed[i];
        } else {
            buffer[i] = payload[i - 4];
        }
    }

    nsapi_size_t bytes_to_send = (index + 4);
    // printf("Size of packet is: %d\n", bytes_to_send);

    _result = _socket.send(buffer, bytes_to_send);
    if(_result < 0) {
        printf("Subscription unsuccessful! Error: %d\n", _result);;
        return false;
    }
    // printf("sent %d bytes\n", _result);
    printf("Subscribed\n");
    return true;
}

bool MQTTclient::disconnect() {
    
    uint8_t buffer[] = {0xE0, 0x00};
    nsapi_size_t bytes_to_send = sizeof(buffer);
    // printf("Size of packet is: %d\n", bytes_to_send);

    _result = _socket.send(buffer, bytes_to_send);
    if(_result < 0) {
        printf("Disconnect unsuccessful! Error:%d\n", _result);
        return false;
    }

    _socket.close();
    _interface->disconnect();

    // printf("sent %d bytes\n", bytes_sent);
    printf("Disconnected\n");
    return true;

}

void MQTTclient::ping() {
    uint8_t buffer[] = {0xC0, 0x00};
    nsapi_size_t bytes_to_send = sizeof(buffer);
    //printf("Size of packet is: %d\n", bytes_to_send);
    nsapi_size_or_error_t bytes_sent = 0;

    bytes_sent = _socket.send(buffer, bytes_to_send);
    //printf("sent %d bytes\n", bytes_sent);
    printf("Ping\n");
    //receive_response();
}

MQTTclient::~MQTTclient() {
    disconnect();
}