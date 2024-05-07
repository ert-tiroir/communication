
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
    if (WiFi.status() == WL_CONNECTED && esp_wifi_client.connected())
        return 1;
    if (WiFi.status() != WL_CONNECTED) {
        __log_info("Connecting to WiFi");
        if (esp_wifi_client.connected())
            esp_wifi_client.stop();

        __log_debug({
            slogger("WiFi parameters are ssid='");
            slogger(esp_wifi_ssid);
            slogger("', password='");
            slogger(esp_wifi_password);
            slogger("'");
        })
        int status = WiFi.begin(esp_wifi_ssid, esp_wifi_password);
        int retry  = 10;
        while (status != WL_CONNECTED) {
            delay(100);
            if (retry == 0) {
                __log_danger("Could not connect to WiFi, timed out after 1 second");
                __log_danger({
                    slogger("WiFi parameters are ssid='");
                    slogger(esp_wifi_ssid);
                    slogger("', password='");
                    slogger(esp_wifi_password);
                    slogger("'");
                })
                return 0;
            }
            retry --;
            continue ;
        }

        __log_debug("Successfully connected to WiFi");
    }

    if (esp_wifi_client.connected()) return 1;

    __log_info("Connecting to server...");

    esp_wifi_client.connect(ip, port);
    while (!esp_wifi_client.connected()) delay(10);

    __log_info("Successfully connected to server.");

    return 1;
}
int esp_wifi_server_connect () {
    WiFiClient newClient = server.available();
    if (!newClient) return esp_wifi_client.connected();

    __log_info("New client connected itself to the network.");
    esp_wifi_client = newClient;
    return 1;
}
int esp_wifi_connect () {
    if (esp_wifi_is_client) return esp_wifi_client_connect();
    return esp_wifi_server_connect();
}

int esp_wifi_server_init (struct buffer_t *_rxbuf, const char* ssid, const char* password, int port) {
    log_info("Init ESP WiFi Server");

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
    delay(1000);

    __log_info({
        slogger("Starting WiFi Server at port ");
        ilogger(port);
    });
    esp_wifi_server.begin(port);
    return 1;
}
int esp_wifi_client_init (struct buffer_t *_rxbuf, const char* _ssid, const char* _password, IPAddress _ip, int _port) {
    esp_wifi_is_client = 1;
    esp_wifi_host_ip = _ip; esp_wifi_host_port = _port;

    esp_wifi_ssid = _ssid;
    esp_wifi_password = _password;

    esp_wifi_rxbuf = _rxbuf;

    __log_info("Starting ESP WiFi Client");
    __log_debug("Setting Mode to STA");
    WiFi.mode(WIFI_STA);
    esp_wifi_client_connect();
    return 1;
}
int esp_wifi_load (struct buffer_t *buffer, unsigned int page_amount) {
    if (esp_wifi_pgamn != 0) return 0;
    __log_debug("New buffer was loaded into ESP WiFi client");
    esp_wifi_txbuf = buffer;
    esp_wifi_pgamn = page_amount;
    return 1;
}

int esp_wifi_recv () {
    unsigned char* rxpage = writable_page(esp_wifi_rxbuf);
    if (rxpage == 0) {
        __log_debug("No more pages to receive the data into");
        return 0;
    }
    if (esp_wifi_client.available() < esp_wifi_rxbuf->pag_size) return 0;

    __log_debug("Received a page from the client, reading it into the buffer");
    esp_wifi_client.read(rxpage, esp_wifi_rxbuf->pag_size);

    free_write(esp_wifi_rxbuf);
    return 1;
}
void esp_wifi_send (unsigned char* buffer, unsigned int size) {
    __log_debug("Writing data to the client");
    int amount = 0;
    while (size > 0) {
        if (!esp_wifi_connect()) continue ;

        int amount = esp_wifi_client.write(buffer, size);
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

    if (esp_wifi_pgamn != 0) {
        unsigned char* write_page = readable_page(esp_wifi_txbuf);
        if (write_page == 0) return 1;

        __log_debug("Sending page to the other side");
    
        esp_wifi_send(write_page, esp_wifi_txbuf->pag_size);

        free_read(esp_wifi_txbuf);
        esp_wifi_pgamn --;
    }

    return 1;
}
