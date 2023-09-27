#include "weather_map.h"

#include <hardware/dma.h>
#include <hardware/spi.h>

weather_map::weather_map()
    : m_timestamp(0)
    , m_nowcast(false)
    , m_data({0})
{}

time_t weather_map::timestamp() const {
    return m_timestamp;
}

weather_map& weather_map::operator=(const weather_map_ext& other) {
    m_timestamp = other.m_timestamp;
    m_nowcast = other.m_nowcast;
    // spi read 4096 bytes from other.m_address to m_data
    return *this;
}

weather_map_ext weather_map::save(uint32_t address) {
    weather_map_ext to_return;
    to_return.m_timestamp = m_timestamp;
    to_return.m_nowcast = m_nowcast;
    to_return.m_address = address;
    // spi write 4096 bytes from m_data to address
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

uint8_t *weather_map::data() {
    return m_data;
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