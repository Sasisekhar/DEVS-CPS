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

    SocketAddress address, ntp;
    
    address.set_ip_address("134.117.52.253\0");
    address.set_port(1883);

    MQTTclient client(interface, address);
    
    if(client.connect("ARSLAB")) {
        printf("Connection Successful\n");
    }

    if(client.subscribe("ARSLAB/Control/AC")) {
        printf("Subscription successful\n");
    }

    if(client.subscribe("ARSLAB/Control/Door")) {
        printf("Subscription successful\n");
    }

    if(client.subscribe("ARSLAB/Control/Light")) {
        printf("Subscription successful\n");
    }

    uint64_t startTime = 0;
    while (true) {

        if(!client.connected()) {
            client.disconnect();
            if(client.connect("ARSLAB")) {
                printf("Connection Successful\n");
            }
        }

        if(arduino::millis() -  startTime > 500) {
            int temp = rand()%50;
            int hum = rand()%100;
            int co2 = rand()%5000;

            char buff[128];

            sprintf(buff, "{\"Temp\":%d, \"Hum\":%d, \"CO2\":%d}", temp, hum, co2);

            client.publish("ARSLAB/Data/Raw", buff);
            startTime = arduino::millis();
        }

        client.receive_response();

        if(button) {
            client.ping();
            ThisThread::sleep_for(500ms);
            if(button){
                break;
            }
        }
    }
    
    printf("End\n");
    return 0;
}