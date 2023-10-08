#include "MC_23LCV1024.h"
#include "crc32.h"

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <pico/binary_info.h>

#include <stdio.h>
#include "logger.h"

#define READ_OP  0x03
#define WRITE_OP 0x02
#define RDMR_OP  0x05
#define WRMR_OP  0x01

static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(SPI_CS, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(SPI_CS, 1);  // Active low
    asm volatile("nop \n nop \n nop");
}

MC_23LCV1024::MC_23LCV1024(uint32_t requested_baud) {
    if(m_baud == 0) {
        m_baud = spi_init(spi0, requested_baud);
        gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
        gpio_set_function(SPI_TX, GPIO_FUNC_SPI);
        gpio_set_function(SPI_RX, GPIO_FUNC_SPI);
        bi_decl(bi_3pins_with_func(SPI_SCK, SPI_RX, SPI_TX, GPIO_FUNC_SPI));

        gpio_init(SPI_CS);
        gpio_set_dir(SPI_CS, GPIO_OUT);
        gpio_put(SPI_CS, 1);
        // Make the CS pin available to picotool
        bi_decl(bi_1pin_with_name(SPI_CS, "SPI CS"));
    }
    cs_deselect();
}

int32_t MC_23LCV1024::read(uint32_t addr, std::span<uint8_t> dst) {
    if(addr >= SIZE) {
        return -1;
    }
    uint8_t buf[4] = {READ_OP, (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF)};
    cs_select();
    spi_write_blocking(spi0, buf, 4);
    int32_t bytes_read = spi_read_blocking(spi0, 0x00, dst.data(), dst.size());
    cs_deselect();
    return bytes_read;
}

int32_t MC_23LCV1024::write(uint32_t addr, std::span<uint8_t> src) {
    if(addr >= SIZE) {
        return -1;
    }
    uint8_t buf[4] = {WRITE_OP, (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF)};
    cs_select();
    spi_write_blocking(spi0, buf, 4);
    int32_t bytes_written = spi_write_blocking(spi0, src.data(), src.size());
    cs_deselect();
    return bytes_written;
}

uint32_t MC_23LCV1024::validate(uint32_t addr, uint32_t size) {
    if(addr >= SIZE) {
        return 0;
    }
    uint8_t buf[4] = {READ_OP, (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF)};
    uint32_t sram_crc = 0xFFFFFFFF;
    uint8_t octet;
    cs_select();
    spi_write_blocking(spi0, buf, 4);
    for(uint32_t i = 0; i < size; i++) {
        spi_read_blocking(spi0, 0x00, &octet, 1);
        sram_crc = updateCRC32(octet, sram_crc);
    }
    cs_deselect();
    return ~sram_crc;
}

void MC_23LCV1024::clear() {
    info1("Clearing SPI SRAM...\n");
    uint8_t buf[4] = {WRITE_OP, 0x00, 0x00, 0x00};
    uint8_t clearbuf[256];
    cs_select();
    spi_write_blocking(spi0, buf, 4);
    int32_t bytes_written = 0;
    while(bytes_written < SIZE) {
        bytes_written += spi_read_blocking(spi0, 0, clearbuf, sizeof(clearbuf));
    }
    cs_deselect();
    info1("Cleared SPI SRAM.\n");
}

void MC_23LCV1024::set_mode(MC_23LCV1024::mode new_mode) {
    uint8_t buf[2] = {WRMR_OP, (uint8_t)new_mode};
    cs_select();
    spi_write_blocking(spi0, buf, 2);
    cs_deselect();
}

MC_23LCV1024::mode MC_23LCV1024::get_mode() {
    uint8_t src[2] = {WRMR_OP, 0x00};
    uint8_t dst[2] = {WRMR_OP, 0x00};
    cs_select();
    spi_write_read_blocking(spi0, src, dst, 2);
    cs_deselect();

    return (MC_23LCV1024::mode)dst[1];
}

uint32_t MC_23LCV1024::m_baud = 0;