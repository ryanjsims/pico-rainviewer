#pragma once

#include <stdint.h>
#include <span>
#include <coroutine>

#define SPI_RX  16
#define SPI_CS  17
#define SPI_SCK 18
#define SPI_TX  19

#if defined(__cpp_impl_coroutine) && defined(__cpp_lib_coroutine)
template <typename T>
struct data_streamer {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type
    {
        T value;
        data_streamer get_return_object();
        std::suspend_always initial_suspend();
        std::suspend_always final_suspend() noexcept;
        void unhandled_exception();

        template <std::convertible_to<T> From>
        std::suspend_always yield_value(From&& from);
        void return_void();
    };

    handle_type handle;
    data_streamer(handle_type h);
    ~data_streamer();
    explicit operator bool();
    T operator()();

private:
    bool full = false;
    void fill();
};
#endif

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

#if defined(__cpp_impl_coroutine) && defined(__cpp_lib_coroutine)
    data_streamer<uint8_t> stream(uint32_t addr, uint32_t size);
#endif

    void set_mode(mode new_mode);
    mode get_mode();

    uint32_t baud() {
        return m_baud;
    }

    static const uint32_t SIZE = 0x20000;
private:
    static uint32_t m_baud;
};

#if defined(__cpp_impl_coroutine) && defined(__cpp_lib_coroutine)
template class data_streamer<uint8_t>;
#endif