#include "MC_23LCV1024.h"

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/sync.h>
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

        info("Initialized spi baud rate at %d Hz\n", m_baud);
    }
}

int32_t MC_23LCV1024::read(uint32_t addr, std::span<uint8_t> dst) {
    if(addr >= SIZE) {
        return -1;
    }
    uint8_t buf[4] = {READ_OP, (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF)};
    uint32_t state = save_and_disable_interrupts();
    cs_select();
    spi_write_blocking(spi0, buf, 4);
    int32_t bytes_read = spi_read_blocking(spi0, 0x00, dst.data(), dst.size());
    cs_deselect();
    restore_interrupts(state);
    return bytes_read;
}

#if defined(__cpp_impl_coroutine) && defined(__cpp_lib_coroutine)
data_streamer<uint8_t> MC_23LCV1024::stream(uint32_t addr, uint32_t size) {
    uint8_t data;
    uint32_t count = 0;
    uint8_t buf[4] = {READ_OP, (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF)};
    cs_select();
    spi_write_blocking(spi0, buf, 4);
    while(count < size) {
        count += spi_read_blocking(spi0, 0x00, &data, sizeof(data));
        co_yield data;
    }
    cs_deselect();
}
#endif

int32_t MC_23LCV1024::write(uint32_t addr, std::span<uint8_t> src) {
    if(addr >= SIZE) {
        return -1;
    }
    uint8_t buf[4] = {WRITE_OP, (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF)};
    uint32_t state = save_and_disable_interrupts();
    cs_select();
    spi_write_blocking(spi0, buf, 4);
    int32_t bytes_written = spi_write_blocking(spi0, src.data(), src.size());
    cs_deselect();
    restore_interrupts(state);
    return bytes_written;
}

void MC_23LCV1024::clear() {
    info1("Clearing SPI SRAM...\n");
    uint8_t buf[4] = {WRITE_OP, 0x00, 0x00, 0x00};
    uint8_t clearbuf[256];
    uint32_t state = save_and_disable_interrupts();
    cs_select();
    spi_write_blocking(spi0, buf, 4);
    int32_t bytes_written = 0;
    while(bytes_written < SIZE) {
        bytes_written += spi_read_blocking(spi0, 0, clearbuf, sizeof(clearbuf));
    }
    cs_deselect();
    restore_interrupts(state);
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


template <typename T>
data_streamer<T>::data_streamer(handle_type h): handle(h) {}

template <typename T>
data_streamer<T>::~data_streamer() {
    handle.destroy();
}

template <typename T>
data_streamer<T>::operator bool() {
    fill();
    return !handle.done();
}

template <typename T>
T data_streamer<T>::operator()() {
    fill();
    full = false;
    return std::move(handle.promise().value);
}

template <typename T>
void data_streamer<T>::fill() {
    if (!full) {
        handle();
        full = true;
    }
}

template <typename T>
data_streamer<T> data_streamer<T>::promise_type::get_return_object() {
    return data_streamer(handle_type::from_promise(*this));
}

template <typename T>
std::suspend_always data_streamer<T>::promise_type::initial_suspend() {
    return {};
}

template <typename T>
std::suspend_always data_streamer<T>::promise_type::final_suspend() noexcept {
    return {};
}

template <typename T>
void data_streamer<T>::promise_type::unhandled_exception() {}

template <typename T>
template <std::convertible_to<T> From>
std::suspend_always data_streamer<T>::promise_type::yield_value(From&& from) {
    value = std::forward<From>(from);
    return {};
}

template <typename T>
void data_streamer<T>::promise_type::return_void() {}
