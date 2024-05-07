
#include "communication/esp_wifi.h"

WiFiServer server;
WiFiClient client; int hasClient = 0;

const char* ssid;
const char* password;
IPAddress ip; int port;

int isClient = 0;

struct buffer_t* rxbuf;
struct buffer_t* txbuf;
unsigned int amount = 0;

int esp_wifi_client_connect () {
    if (WiFi.status() == WL_CONNECTED && client.connected())
        return 1;
    if (WiFi.status() != WL_CONNECTED) {
        int status = WiFi.begin(ssid, password);
        int retry  = 10;
        while (status != WL_CONNECTED) {
            delay(100);
            if (retry == 0) return 0;
            retry --;
            continue ;
        }
    }

    if (client.connected()) return 1;

    client.connect(ip, port);
    while (!client.connected()) delay(10);

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

int esp_wifi_server_init (struct buffer_t *_rxbuf, const char* ssid, const char* password, int port) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    rxbuf = _rxbuf;

    IPAddress ip(192, 168, 0, 1);
    IPAddress nm(255, 255, 255, 0);

    WiFi.softAPConfig(ip, ip, nm);
    delay(1000);

    server.begin(port);
    return 1;
}
int esp_wifi_client_init (struct buffer_t *_rxbuf, const char* _ssid, const char* _password, IPAddress _ip, int _port) {
    isClient = 1;
    ip = _ip; port = _port;

    ssid = _ssid;
    password = _password;

    rxbuf = _rxbuf;

    WiFi.mode(WIFI_STA);
    esp_wifi_client_connect();
    return 1;
}
int esp_wifi_load (struct buffer_t *buffer, unsigned int page_amount) {
    if (amount != 0) return 0;
    txbuf = buffer;
    amount = page_amount;
    return 1;
}

int esp_wifi_recv () {
    unsigned char* rxpage = writable_page(rxbuf);
    if (rxpage == 0) return 0;
    if (client.available() < rxbuf->pag_size) return 0;

    client.read(rxpage, rxbuf->pag_size);

    free_write(rxbuf);
    return 1;
}
void esp_wifi_send (unsigned char* buffer, unsigned int size) {
    int amount = 0;
    while (size > 0) {
        if (!esp_wifi_connect()) continue ;

        int amount = client.write(buffer, size);
        buffer += amount;
        size   -= amount;
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
        unsigned char* write_page = readable_page(txbuf);
        if (write_page == 0) return 1;
    
        esp_wifi_send(write_page, txbuf->pag_size);

        free_read(txbuf);
        amount --;
    }

    return 1;
}
