#pragma once

#include "communication/buffer.h"

int esp_spi_init (struct buffer_t &rxbuf);
int esp_spi_load (struct buffer_t &buffer, unsigned int page_amount);
int esp_spi_tick ();
