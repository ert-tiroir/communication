
#include "communication/rpi_spi.h"
#include "wiringPi.h"
#include "wiringPiSPI.h"

const int SPI_channel   = 0;
const int SPI_CLK_SPEED = 16000000;

const int SPI_AVL_0 = 4;
const int SPI_AVL_1 = 5;

const int SPI_AVL_size = 2;

const int SPI_DS    = 15;
const int SPI_DM_BS = 16;
const int SPI_DM_AS = 11;

struct buffer_t *rxbuf = 0;
struct buffer_t *txbuf = 0;
unsigned int     txamn = 0;

int SPI_AVL_offset = -1;

int rpi_spi_init (struct buffer_t *buffer) {
    rxbuf = buffer;
    
    pinMode(SPI_AVL_0, INPUT);
    pinMode(SPI_AVL_1, INPUT);
    pinMode(SPI_DS,    INPUT);
    pinMode(SPI_DM_BS, OUTPUT);
    pinMode(SPI_DM_AS, OUTPUT);

    int fd = wiringPiSPISetupMode(SPI_channel, SPI_CLK_SPEED, 0);
    if (fd == -1)
        return 0;
    return 1;
}
int rpi_spi_load (struct buffer_t *buffer, unsigned int page_amount) {
    if (txamn != 0) return 0;

    txamn = page_amount;
    txbuf = buffer;
    return 1;
}
int rpi_spi_tick () {
    unsigned char* rx_page = writable_page(rxbuf);
    if (!rx_page) return 0;
    
    int data_master = txamn != 0 && readable_page(txbuf);
    int data_slave  = digitalRead(SPI_DS);
    if (data_master) {
        unsigned char* tx_page = readable_page(txbuf);

        for (int i = 0; i < pag_size(txbuf); i ++)
            rx_page[i] = tx_page[i];

        txamn --;
        free_read(txbuf);
    }

    if (!(data_master || data_slave)) return 1;

    int SPI_AVL[2] = { SPI_AVL_0, SPI_AVL_1 };

    digitalWrite( SPI_DM_BS, data_master );

    int cnt = 0;
    while (SPI_AVL_offset == -1 || digitalRead(SPI_AVL[SPI_AVL_offset]) == 0) {
        while (SPI_AVL_offset == -1) {
            if      (digitalRead(SPI_AVL_0)) SPI_AVL_offset = 0;
            else if (digitalRead(SPI_AVL_1)) SPI_AVL_offset = 1;
        }

        cnt ++;
        if (cnt >= 1000000) {
            SPI_AVL_offset = -1;
            cnt = 0;
        }
    }

    if (digitalRead(SPI_DS)) data_slave = 1;

    digitalWrite( SPI_DM_AS, data_master );
    digitalWrite( SPI_DM_BS, LOW );

    wiringPiSPIDataRW(SPI_channel, rxbuf, rxbuf->pag_size);

    SPI_AVL_offset ++;
    if (SPI_AVL_offset == SPI_AVL_size) SPI_AVL_offset = 0;    

    if (data_slave)
        free_read(rxbuf);
}
