#include "weather_map.h"

weather_map::weather_map()
    : m_timestamp(0)
    , m_data({0})
{}

time_t weather_map::timestamp() {
    return m_timestamp;
}

uint32_t weather_map::get_color(uint8_t row, uint8_t col, const uint32_t* palette) {
    if(palette == nullptr || row > 63 || col > 63) {
        return 0xFF0F000F;
    }
    return palette[m_data[row * 64 + col]];
}

uint8_t weather_map::get_grayscale(uint8_t row, uint8_t col) {
    if(row > 63 || col > 63) {
        return 0x00;
    }
    return m_data[row * 64 + col];
}

int8_t weather_map::get_dbz(uint8_t row, uint8_t col) {
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