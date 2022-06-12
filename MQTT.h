#ifndef MQTT_h
#define MQTT_h

#include "NetworkInterface.h"
#include "SocketAddress.h"
#include "TCPSocket.h"

#define MQTTCONNECT     1 << 4  // Client request to connect to Server
#define MQTTCONNACK     2 << 4  // Connect Acknowledgment
#define MQTTPUBLISH     3 << 4  // Publish message
#define MQTTPUBACK      4 << 4  // Publish Acknowledgment
#define MQTTPUBREC      5 << 4  // Publish Received (assured delivery part 1)
#define MQTTPUBREL      6 << 4  // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP     7 << 4  // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE   8 << 4  // Client Subscribe request
#define MQTTSUBACK      9 << 4  // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request
#define MQTTUNSUBACK    11 << 4 // Unsubscribe Acknowledgment
#define MQTTPINGREQ     12 << 4 // PING Request
#define MQTTPINGRESP    13 << 4 // PING Response
#define MQTTDISCONNECT  14 << 4 // Client is Disconnecting
#define MQTTReserved    15 << 4 // Reserved

class MQTTclient {
    private:
    NetworkInterface *_interface;// = NetworkInterface::get_default_instance();
    TCPSocket _socket;
    SocketAddress _address;
    nsapi_size_or_error_t _result;
    struct Message {
        char _clientID[64];
        char _topic[64];
        char _payload[64];
    } _global;

    public:
    MQTTclient(NetworkInterface*, SocketAddress);
    ~MQTTclient();
    bool MQTTinit();
    bool connect(const char* username, const char* password, const char* clientID);
    bool connect(const char* clientID);
    void receive_response();
    bool publish(const char* topic, const char* message);
    bool subscribe(const char* topic);
    void ping();
    bool disconnect();
};

#endif