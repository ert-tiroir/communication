
#include "communication/esp_wifi.h"
#include "logger.h"

WiFiServer esp_wifi_server;
WiFiClient esp_wifi_client; int esp_wifi_has_client = 0;

const char* esp_wifi_ssid;
const char* esp_wifi_password;
IPAddress esp_wifi_host_ip; int esp_wifi_host_port;

int esp_wifi_is_client = 0;

struct buffer_t* esp_wifi_rxbuf;
struct buffer_t* esp_wifi_txbuf;
unsigned int esp_wifi_pgamn = 0;

int esp_wifi_client_connect () {
    int status = WiFi.begin(esp_wifi_ssid, esp_wifi_password);
    if (status != WL_CONNECTED) return 0;

    if (esp_wifi_client.connected()) return 1;

    esp_wifi_client.connect(esp_wifi_host_ip, esp_wifi_host_port);

    return 1;
}
int esp_wifi_server_connect () {
    WiFiClient newClient = esp_wifi_server.available();
    if (!newClient) return esp_wifi_client.connected();

    esp_wifi_client = newClient;
    return 1;
}
int esp_wifi_connect () {
    if (esp_wifi_is_client) return esp_wifi_client_connect();
    return esp_wifi_server_connect();
}

int esp_wifi_server_init (struct buffer_t *_rxbuf, const char* ssid, const char* password, int port) {
    log_info("Init WiFi Server");

    log_debug("Start WiFi AP");
    WiFi.mode(WIFI_AP);
    __log_debug({
        slogger("Starting SoftAP with ssid='");
        slogger(ssid);
        slogger("', password='");
        slogger(password);
        slogger("'");
    })
    WiFi.softAP(ssid, password);

    esp_wifi_rxbuf = _rxbuf;

    log_debug("SoftAP Config with host at 192.168.0.1 and mask at 255.255.255.0");
    IPAddress ip(192, 168, 0, 1);
    IPAddress nm(255, 255, 255, 0);

    WiFi.softAPConfig(ip, ip, nm);

    __log_info({
        slogger("Starting WiFi Server at port ");
        ilogger(port);
    });
    esp_wifi_server.begin(port);
    return 1;
}
int esp_wifi_client_init (struct buffer_t *_rxbuf, const char* ssid, const char* password, IPAddress _ip, int _port) {
    esp_wifi_is_client = 1;
    esp_wifi_host_ip = _ip; esp_wifi_host_port = _port;

    esp_wifi_rxbuf = _rxbuf;

    WiFi.mode(WIFI_STA);
    esp_wifi_client_connect();
    return 1;
}
int esp_wifi_load (struct buffer_t *buffer, unsigned int page_amount) {
    if (esp_wifi_pgamn != 0) return 0;
    esp_wifi_txbuf = buffer;
    esp_wifi_pgamn = page_amount;
    return 1;
}

int esp_wifi_recv () {
    unsigned char* rxpage = readable_page(esp_wifi_rxbuf);
    if (!rxpage) return 0;
    if (esp_wifi_client.available() < esp_wifi_rxbuf->pag_size) return 0;

    esp_wifi_client.read(rxpage, esp_wifi_rxbuf->pag_size);

    free_read(esp_wifi_rxbuf);
    return 1;
}
void esp_wifi_send (unsigned char* buffer, unsigned int size) {
    int amount = 0;
    while (size >= 0) {
        if (!esp_wifi_connect()) continue ;

        size -= esp_wifi_client.write(buffer, size);
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

    if (esp_wifi_pgamn != 0) {
        unsigned char* write_page = writable_page(esp_wifi_txbuf);
        if (!write_page) return 1;
    
        esp_wifi_send(write_page, esp_wifi_txbuf->pag_size);

        free_write(esp_wifi_txbuf);
        esp_wifi_pgamn --;
    }

    return 1;
}
