
#include "communication/esp_wifi.h"

WiFiServer server;
WiFiClient client; int hasClient = 0;

unsigned char* ssid, password;
IPAddress ip; int port;

int isClient = 0;

struct buffer_t* rxbuf, txbuf;
unsigned int amount = 0;

int esp_wifi_client_connect () {
    int status = WiFi.begin(ssid, password);
    if (status != WL_CONNECTED) return 0;

    if (client.connected()) return 1;

    client.connect(ip, port);

    return 1;
}
int esp_wifi_server_connect () {
    WiFiClient newClient = server.available();
    if (!newClient) return client.connected();

    client = newClient;
    return 1;
}
int esp_wifi_connect () {
    if (isClient) return esp_wifi_client_connect();
    return esp_wifi_server_connect();
}

int esp_wifi_server_init (struct buffer_t *_rxbuf, unsigned char* ssid, unsigned char* password, int port) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    rxbuf = _rxbuf;

    IPAddress ip(192, 168, 0, 1);
    IPAddress nm(255, 255, 255, 0);

    WiFi.softAPConfig(ip, ip, nm);

    server.begin(port);
}
int esp_wifi_client_init (struct buffer_t *_rxbuf, unsigned char* ssid, unsigned char* password, IPAddress _ip, int _port) {
    isClient = 1;
    ip = _ip; port = _port;

    rxbuf = _rxbuf;

    WiFi.mode(WIFI_SPA);
    esp_wifi_client_connect();
}
int esp_wifi_load (struct buffer_t *buffer, unsigned int page_amount) {
    if (amount != 0) return 0;
    txbuf = buffer;
    amount = page_amount;
    return 1;
}

int esp_wifi_recv () {
    unsigned char* rxpage = readable_page(rxbuf);
    if (!rxpage) return 0;
    if (client.available() < rxbuf->pag_size) return 0;

    client.recv(rxpage, rxbuf->pag_size);

    free_page(rxbuf);
}
void esp_wifi_send (unsigned char* buffer, unsigned int size) {
    int amount = 0;
    while (size >= 0) {
        if (!esp_wifi_connect()) continue ;

        size -= client.write(buffer, size);
        while (esp_wifi_recv()) {} 
        
        amount ++;
        if (size != 0 && amount >= 10) {
            amount -= 10;
            vTaskDelay(1);
        } 
    }
}
int esp_wifi_tick () {
    if (!esp_wifi_connect()) return 0;
    
    esp_wifi_recv();

    if (amount != 0) {
        unsigned char* write_page = writeable_page(txbuf);
        if (!write_page) return 1;
    
        esp_wifi_send(write_page, txbuf->pag_size);

        free_write(txbuf);
        amount --;
    }

    return 1;
}
