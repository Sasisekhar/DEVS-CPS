#include "MQTT.h"
#include "DigitalIn.h"
#include "mbed.h"

namespace arduino{
    uint64_t millis() {
        return (us_ticker_read()/1000);
    }
}

void resolve_hostname(NetworkInterface *interface, SocketAddress &address) {
    const char hostname[] = "broker.hivemq.com";

    /* get the host address */
    printf("\nResolve hostname %s\r\n", hostname);
    nsapi_size_or_error_t result = interface->gethostbyname(hostname, &address);
    if (result != 0) {
        printf("Error! gethostbyname(%s) returned: %d\r\n", hostname, result);
    }

    printf("%s address is %s\r\n", hostname, (address.get_ip_address() ? address.get_ip_address() : "None") );
}

void networkInfo(NetworkInterface *interface) {
    SocketAddress a;
    interface->get_ip_address(&a);
    printf("IP address: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
    printf("MAc Address: %s\r\n", interface->get_mac_address() ? interface->get_mac_address() : "None");
    interface->get_netmask(&a);
    printf("Netmask: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
    interface->get_gateway(&a);
    printf("Gateway: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
}

// main() runs in its own thread in the OS
int main() {

    //Create a network interface and connect to the network
    NetworkInterface *interface = NetworkInterface::get_default_instance();
    interface->connect();
    networkInfo(interface);

    SocketAddress address;
    //resolve_hostname(network::address);
    address.set_ip_address("134.117.52.253\0");
    address.set_port(1883);

    MQTTclient client(interface, address);

    if(!client.MQTTinit()) {
        printf("MQTT initialization failed!");
        return 0;
    }


    DigitalIn button(BUTTON1);   

    printf("Connecting to the network...\r\n");
    
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