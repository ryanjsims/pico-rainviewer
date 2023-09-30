#pragma once

#include <stdint.h>
#include <span>

#define SPI_RX  16
#define SPI_CS  17
#define SPI_SCK 18
#define SPI_TX  19

class MC_23LCV1024 {
public:
    enum class mode: uint8_t {
        byte = 0,
        page = 0x80,
        full = 0x40,
        resv = 0xC0
    };
    MC_23LCV1024(uint32_t requested_baud);

    int32_t read(uint32_t addr, std::span<uint8_t> dst);
    int32_t write(uint32_t addr, std::span<uint8_t> src);
    void clear();

    void set_mode(mode new_mode);
    mode get_mode();

    uint32_t baud() {
        return m_baud;
    }

    static const uint32_t SIZE = 0x20000;
private:
    static uint32_t m_baud;
};