#include "weather_map.h"
#include "crc32.h"

#include <stdio.h>
#include <string.h>

#include "logger.h"

weather_map::weather_map(MC_23LCV1024* sram)
    : m_timestamp(0)
    , m_nowcast(false)
    , m_data{0}
    , m_sram(sram)
{
    if(m_sram->get_mode() != MC_23LCV1024::mode::full) {
        m_sram->set_mode(MC_23LCV1024::mode::full);
    }
    info("SPI baud set to: %d\n", m_sram->baud());
}

time_t weather_map::timestamp() const {
    return m_timestamp;
}

weather_map& weather_map::load(const weather_map_ext& other, bool load) {
    m_timestamp = other.m_timestamp;
    m_nowcast = other.m_nowcast;
    if(load) {
        // spi read 4096 bytes from other.m_address to m_data
        int32_t read = m_sram->read(other.address(), {m_data, sizeof(m_data)});
        debug("Read %d bytes from 0x%05x\n", read, other.address());
    } else {
        debug1("Not loading from sram.\n");
    }
    return *this;
}

weather_map_ext weather_map::save(uint32_t address) {
    weather_map_ext to_return;
    to_return.m_timestamp = m_timestamp;
    to_return.m_nowcast = m_nowcast;
    to_return.m_address = address;
    to_return.m_crc32 = crc32buf((char*)m_data, sizeof(m_data));
    uint32_t sram_crc = ~to_return.m_crc32;
    int32_t wrote = 0;
    for(uint8_t i = 3; i > 0 && sram_crc != to_return.m_crc32; i--) {
        if(sram_crc != to_return.m_crc32 && wrote > 0) {
            warn("Invalid CRC: actual 0x%08x != expected 0x%08x. (retries left: %d)\n", sram_crc, to_return.m_crc32, i);
        }
        // spi write 4096 bytes from m_data to address
        wrote = m_sram->write(address, {m_data, sizeof(m_data)});
        sram_crc = m_sram->validate(address, sizeof(m_data));
    }
    if(sram_crc != to_return.m_crc32) {
        error("Invalid CRC: actual 0x%08x != expected 0x%08x, giving up\n", sram_crc, to_return.m_crc32);
    }
    info("Wrote %d bytes to 0x%05x\n", wrote, address);
    to_return.m_init = true;
    return to_return;
}

bool weather_map::nowcast() const {
    return m_nowcast;
}

void weather_map::set_nowcast(bool is_nowcast) {
    m_nowcast = is_nowcast;
}

uint32_t weather_map::get_color(uint8_t row, uint8_t col, const uint32_t* palette) const {
    if(palette == nullptr || row > 63 || col > 63) {
        return 0xFF0F000F;
    }
    return palette[m_data[row * 64 + col]];
}

uint8_t weather_map::get_grayscale(uint8_t row, uint8_t col) const {
    if(row > 63 || col > 63) {
        return 0x00;
    }
    return m_data[row * 64 + col];
}

int8_t weather_map::get_dbz(uint8_t row, uint8_t col) const {
    if(row > 63 || col > 63) {
        return 0x00;
    }
    return ((int8_t)m_data[row * 64 + col] & 0x7F) - 32;
}

void weather_map::set_pixel(uint8_t row, uint8_t col, uint8_t value) {
    if(row > 63 || col > 63) {
        return;
    }
    m_data[row * 64 + col] = value;
}

uint8_t *weather_map::data() {
    return m_data;
}

void weather_map::dump(uint8_t number) const {
    info("Weather map %d dump:\n", number);
    for(int row = 0; row < 64; row++) {
        for(int col = 0; col < 64; col++) {
            if(col % 8 == 0 && col != 0) {
                printf(" ");
            }
            printf("%02x ", m_data[row * 64 + col]);
        }
        printf("\n");
    }
}

void weather_map::clear() {
    memset(m_data, 0x00, sizeof(m_data));
    m_sram->clear();
}

void weather_map::set_timestamp(time_t new_timestamp) {
    m_timestamp = new_timestamp;
}

weather_map_ext::weather_map_ext()
    : m_timestamp(0)
    , m_nowcast(false)
    , m_init(false)
    , m_address(0)
{}

time_t weather_map_ext::timestamp() const {
    return m_timestamp;
}

bool weather_map_ext::nowcast() const {
    return m_nowcast;
}

bool weather_map_ext::initialized() const {
    return m_init;
}

void weather_map_ext::set_nowcast(bool is_nowcast) {
    m_nowcast = is_nowcast;
}

void weather_map_ext::set_timestamp(time_t new_timestamp) {
    m_timestamp = new_timestamp;
}

uint32_t weather_map_ext::address() const {
    return m_address;
}