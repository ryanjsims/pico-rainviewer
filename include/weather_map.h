#pragma once

#include <stdint.h>
#include <time.h>

class weather_map {
public:
    weather_map();

    time_t timestamp();
    uint32_t get_color(uint8_t row, uint8_t col, const uint32_t* palette);
    uint8_t get_grayscale(uint8_t row, uint8_t col);
    int8_t get_dbz(uint8_t row, uint8_t col);

    uint8_t *data();
    void set_timestamp(time_t new_timestamp);
private:
    time_t m_timestamp;
    uint8_t m_data[4096];
};