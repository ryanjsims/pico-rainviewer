#pragma once

#include <stdint.h>
#include <time.h>

class weather_map_ext;

class weather_map {
public:
    weather_map();

    weather_map& operator=(const weather_map_ext& other);
    weather_map_ext save(uint32_t address);

    time_t timestamp() const;
    void set_timestamp(time_t new_timestamp);

    bool nowcast() const;
    void set_nowcast(bool is_nowcast);

    uint32_t get_color(uint8_t row, uint8_t col, const uint32_t* palette) const;
    uint8_t get_grayscale(uint8_t row, uint8_t col) const;
    int8_t get_dbz(uint8_t row, uint8_t col) const;

    uint8_t *data();
private:
    time_t m_timestamp;
    bool m_nowcast;
    uint8_t m_data[4096];
};

class weather_map_ext {
    friend class weather_map;
public:
    weather_map_ext();

    time_t timestamp() const;
    void set_timestamp(time_t new_timestamp);

    bool nowcast() const;
    void set_nowcast(bool is_nowcast);

    uint32_t address() const;
    bool initialized() const;
private:
    time_t m_timestamp;
    bool m_nowcast, m_init;
    uint32_t m_address;
};