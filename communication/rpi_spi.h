#pragma once
#include "communication/buffer.h"

int rpi_spi_init (struct buffer_t *rxbuf);
int rpi_spi_load (struct buffer_t *buffer, unsigned int page_amount);
int rpi_spi_tick ();
