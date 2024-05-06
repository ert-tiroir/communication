#pragma once

#include "communication/buffer.h"
#include "WiFi.h"

int esp_wifi_server_init (struct buffer_t *rxbuf, unsigned char* ssid, unsigned char* password, int port);
int esp_wifi_client_init (struct buffer_t *rxbuf, unsigned char* ssid, unsigned char* password, IPAddress ip, int port);
int esp_wifi_load (struct buffer_t *buffer, unsigned int page_amount);
int esp_wifi_tick ();
