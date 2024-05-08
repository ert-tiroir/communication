
#include "communication/rpi_spi.h"
#include "wiringPi.h"
#include "wiringPiSPI.h"
#include "logger.h"

const int rpi_spi_channel   = 0;
const int rpi_spi_clk_speed = 16000000;

const int rpi_spi_avl_0 = 4;
const int rpi_spi_avl_1 = 5;

const int rpi_spi_avl_size = 2;

const int rpi_spi_ds    = 15;
const int rpi_spi_dm_bs = 16;
const int rpi_spi_dm_as = 1;

struct buffer_t *rpi_spi_rxbuf = 0;
struct buffer_t *rpi_spi_txbuf = 0;
unsigned int     rpi_spi_pgamn = 0;

int rpi_spi_avl_offset = -1;

int rpi_spi_init (struct buffer_t *buffer) {
    rpi_spi_rxbuf = buffer;
    
    log_info("Initialize RPI SPI");
    log_debug("Init GPIO");
    pinMode(rpi_spi_avl_0, INPUT);
    pinMode(rpi_spi_avl_1, INPUT);
    pinMode(rpi_spi_ds,    INPUT);
    pinMode(rpi_spi_dm_bs, OUTPUT);
    pinMode(rpi_spi_dm_as, OUTPUT);

    log_info("Init SPI");
    int fd = wiringPiSPISetupMode(rpi_spi_channel, rpi_spi_clk_speed, 0);
    if (fd == -1)
        return 0;
    return 1;
}
int rpi_spi_load (struct buffer_t *buffer, unsigned int page_amount) {
    if (rpi_spi_pgamn != 0) return 0;
    log_debug("Successfully loaded buffer");

    rpi_spi_pgamn = page_amount;
    rpi_spi_txbuf = buffer;
    return 1;
}
int rpi_spi_tick () {
    int SPI_AVL[2] = { rpi_spi_avl_0, rpi_spi_avl_1 };
    log_debug("SPI Tick");
    unsigned char* rx_page = writable_page(rpi_spi_rxbuf);
    if (!rx_page) return 0;
    
    int data_master = rpi_spi_pgamn != 0 && readable_page(rpi_spi_txbuf);
    int data_slave  = digitalRead(rpi_spi_ds) && (
        rpi_spi_avl_offset == -1
        || digitalRead(SPI_AVL[rpi_spi_avl_offset])
    );
    if (data_master) {
        unsigned char* tx_page = readable_page(rpi_spi_txbuf);

        for (int i = 0; i < pag_size(rpi_spi_txbuf); i ++)
            rx_page[i] = tx_page[i];

        rpi_spi_pgamn --;
        free_read(rpi_spi_txbuf);
    }

    if (!(data_master || data_slave)) return 1;

    __log_debug({
        slogger("Preparing SPI Transaction DM=");
        ilogger(data_master);
        slogger(", DS=");
        ilogger(data_slave);
        slogger(", PinDS="); 
        ilogger(digitalRead(rpi_spi_ds));
    })
    digitalWrite( rpi_spi_dm_bs, data_master );

    int cnt = 0;
    while (rpi_spi_avl_offset == -1 || digitalRead(SPI_AVL[rpi_spi_avl_offset]) == 0) {
        while (rpi_spi_avl_offset == -1) {
            if      (digitalRead(rpi_spi_avl_0)) rpi_spi_avl_offset = 0;
            else if (digitalRead(rpi_spi_avl_1)) rpi_spi_avl_offset = 1;
        }

        cnt ++;
        if (cnt >= 1000000) {
            rpi_spi_avl_offset = -1;
            cnt = 0;
        }
    }

    if (digitalRead(rpi_spi_ds)) data_slave = 1;

    digitalWrite( rpi_spi_dm_as, data_master );
    digitalWrite( rpi_spi_dm_bs, LOW );
    
    __log_debug({
        slogger("Starting SPI Transaction DM=");
        ilogger(data_master);
        slogger(", DS=");
        ilogger(data_slave);
        slogger(", PinDS="); 
        ilogger(digitalRead(rpi_spi_ds));
    })

    wiringPiSPIDataRW(rpi_spi_channel, rx_page, rpi_spi_rxbuf->pag_size);
    __log_debug({
        slogger("Ended SPI Transaction DM=");
        ilogger(data_master);
        slogger(", DS=");
        ilogger(data_slave);
    })

    rpi_spi_avl_offset ++;
    if (rpi_spi_avl_offset == rpi_spi_avl_size) rpi_spi_avl_offset = 0;    

    if (data_slave)
        free_write(rpi_spi_rxbuf);
}
