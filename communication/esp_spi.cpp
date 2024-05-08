
#include "communication/esp_spi.h"
#include "driver/spi_slave.h"
#include "Arduino.h"
#include "logger.h"

const int PAGE_SIZE = 1024;

const int SPI_DS    = 6;
const int SPI_DM_BS = 5;
const int SPI_DM_AS = 4;

const int SPI_AVL0 = 17;
const int SPI_AVL1 = 18;

int SPI_AVL[2] = {SPI_AVL0, SPI_AVL1};
int SPI_AVL_offset = 0;

const int SPI_AVL_SIZE = 2;

int spi_slave_has_data  = 0;
int spi_master_has_data = 0;

struct buffer_t* esp_spi_rxbuf;
struct buffer_t* esp_spi_txbuf;
int esp_spi_pgamn = 0;

void spi_slave_post_setup_cb(spi_slave_transaction_t *trans)
{
    digitalWrite(SPI_DS, spi_slave_has_data);
    digitalWrite(SPI_AVL[SPI_AVL_offset], HIGH);
}

void spi_slave_post_trans_cb(spi_slave_transaction_t *trans)
{
    digitalWrite(SPI_AVL[SPI_AVL_offset], LOW);
    digitalWrite(SPI_DS, LOW);
    SPI_AVL_offset++;
    if (SPI_AVL_offset == SPI_AVL_SIZE)
        SPI_AVL_offset = 0;
}

int esp_spi_init (struct buffer_t *rxbuf) {
    log_info("Initializing SPI Slave");
    log_debug("Init GPIO Pins");
    pinMode(SPI_AVL0,  OUTPUT);
    pinMode(SPI_AVL1,  OUTPUT);
    pinMode(SPI_DS,    OUTPUT);
    pinMode(SPI_DM_BS, INPUT);
    pinMode(SPI_DM_AS, INPUT);

    esp_spi_rxbuf = rxbuf;

    log_debug("Init SPI");
    esp_err_t ret;
    spi_bus_config_t buscfg = {
        .mosi_io_num = MOSI,
        .miso_io_num = MISO,
        .sclk_io_num = SCK,
        .data2_io_num = -1,
        .data3_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 4096,
        .flags = SPICOMMON_BUSFLAG_SLAVE,
        .intr_flags = 0};
    spi_slave_interface_config_t slvcfg = {
        .spics_io_num = SS,
        .flags = 0,
        .queue_size = 1,
        .mode = SPI_MODE0,
        .post_setup_cb = spi_slave_post_setup_cb,
        .post_trans_cb = spi_slave_post_trans_cb,
    };
    ret = spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);

    if (ret != ESP_OK)
    {
        log_error("SPI Could not be initialized");
        return 0;
    }

    return 1;
}
int esp_spi_load (struct buffer_t *buffer, unsigned int page_amount) {
    if (esp_spi_pgamn != 0) return 0;
    log_debug("Successfully loaded buffer into SPI");
    esp_spi_txbuf = buffer;
    esp_spi_pgamn = page_amount;
    return 1;
}
int esp_spi_tick () {
    unsigned char* write_page = writable_page(esp_spi_rxbuf);
    if (!write_page) return 0;

    spi_slave_has_data  = LOW;
    spi_master_has_data = 0;

    if (esp_spi_pgamn != 0) {
        unsigned char* page = readable_page(esp_spi_txbuf);

        if (page) {
            for (int i = 0; i < PAGE_SIZE; i ++)
                write_page[i] = page[i];

            spi_slave_has_data = HIGH;

            free_read(esp_spi_txbuf);
            esp_spi_pgamn --;
        }
    }

    if (digitalRead(SPI_DM_BS)) spi_master_has_data = 1;

    if (!(spi_slave_has_data || spi_master_has_data)) return 0;

    spi_slave_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.length    = PAGE_SIZE * 8;
    t.trans_len = PAGE_SIZE * 8;
    t.rx_buffer = write_page;
    t.tx_buffer = write_page;
    __log_debug({
        slogger("Starting transmission, DM=");
        ilogger(spi_master_has_data);
        slogger(", DS=");
        ilogger(spi_slave_has_data);
    });
    esp_err_t ret = spi_slave_transmit(SPI2_HOST, &t, portMAX_DELAY);

    if (digitalRead(SPI_DM_AS)) spi_master_has_data = 1;
    __log_debug({
        slogger("Ended transmission, DM=");
        ilogger(spi_master_has_data);
        slogger(", DS=");
        ilogger(spi_slave_has_data);
    })

    if (spi_master_has_data)
        free_write(esp_spi_rxbuf);
    return 1;
}
