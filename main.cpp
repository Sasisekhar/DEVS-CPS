#include "MQTT.h"
#include "DigitalIn.h"
#include "ThisThread.h"
#include "mbed.h"

namespace arduino{
    uint64_t millis() {
        return (us_ticker_read()/1000);
    }
}

void resolve_hostname(NetworkInterface *interface, SocketAddress &address, const char* url) {
    /* get the host address */
    printf("\nResolve hostname %s\r\n", url);
    nsapi_size_or_error_t result = interface->gethostbyname(url, &address);
    if (result != 0) {
        printf("Error! gethostbyname(%s) returned: %d\r\n", url, result);
    }

    printf("%s address is %s\r\n", url, (address.get_ip_address() ? address.get_ip_address() : "None") );
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

    DigitalIn button(BUTTON1);

    //Create a network interface and connect to the network
    NetworkInterface *interface = NetworkInterface::get_default_instance();
    interface->connect();
    networkInfo(interface);

    SocketAddress address;
    // resolve_hostname(interface, address, "broker.hivemq.com");
    address.set_ip_address("134.117.52.253\0");
    address.set_port(1883);

    //Working till here
    MQTTclient client(interface, address);

    if(!client.MQTTinit()) {
        printf("MQTT initialization failed!");
        return 0;
    }
    
    client.connect("ARSLAB");

    client.subscribe("ARSLAB/Control");

    // printf("Entering loop\n");

    uint64_t startTime = 0;

    while (true) {

        if(arduino::millis() -  startTime > 5000) {
            client.publish("ARSLAB/Temperature", "23");
            startTime = arduino::millis();
        }

        client.receive_response();

        if(button) {
            client.ping(arduino::millis());
            ThisThread::sleep_for(500ms);
            if(button){
                break;
            }
        }
    }
    
    printf("End\n");
    return 0;
}