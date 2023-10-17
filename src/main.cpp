#include <stdio.h>
#include <charconv>
#include <pico/cyw43_arch.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/util/datetime.h>
#include <hardware/rtc.h>
#include <http_client.h>
#include <ntp_client.h>
#include <nlohmann/json.hpp>
#include "logger.h"
#include <PNGdec.h>

#include "color_tables.h"
#include "weather_map.h"
#include "rgb_matrix.h"
#include "MC_23LCV1024.h"
#include "crc32.h"

#pragma message("Building rainviewer for SSID " WIFI_SSID)

const char ISRG_ROOT_X1_CERT[] = "-----BEGIN CERTIFICATE-----\n\
MIIFYDCCBEigAwIBAgIQQAF3ITfU6UK47naqPGQKtzANBgkqhkiG9w0BAQsFADA/\n\
MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n\
DkRTVCBSb290IENBIFgzMB4XDTIxMDEyMDE5MTQwM1oXDTI0MDkzMDE4MTQwM1ow\n\
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n\
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwggIiMA0GCSqGSIb3DQEB\n\
AQUAA4ICDwAwggIKAoICAQCt6CRz9BQ385ueK1coHIe+3LffOJCMbjzmV6B493XC\n\
ov71am72AE8o295ohmxEk7axY/0UEmu/H9LqMZshftEzPLpI9d1537O4/xLxIZpL\n\
wYqGcWlKZmZsj348cL+tKSIG8+TA5oCu4kuPt5l+lAOf00eXfJlII1PoOK5PCm+D\n\
LtFJV4yAdLbaL9A4jXsDcCEbdfIwPPqPrt3aY6vrFk/CjhFLfs8L6P+1dy70sntK\n\
4EwSJQxwjQMpoOFTJOwT2e4ZvxCzSow/iaNhUd6shweU9GNx7C7ib1uYgeGJXDR5\n\
bHbvO5BieebbpJovJsXQEOEO3tkQjhb7t/eo98flAgeYjzYIlefiN5YNNnWe+w5y\n\
sR2bvAP5SQXYgd0FtCrWQemsAXaVCg/Y39W9Eh81LygXbNKYwagJZHduRze6zqxZ\n\
Xmidf3LWicUGQSk+WT7dJvUkyRGnWqNMQB9GoZm1pzpRboY7nn1ypxIFeFntPlF4\n\
FQsDj43QLwWyPntKHEtzBRL8xurgUBN8Q5N0s8p0544fAQjQMNRbcTa0B7rBMDBc\n\
SLeCO5imfWCKoqMpgsy6vYMEG6KDA0Gh1gXxG8K28Kh8hjtGqEgqiNx2mna/H2ql\n\
PRmP6zjzZN7IKw0KKP/32+IVQtQi0Cdd4Xn+GOdwiK1O5tmLOsbdJ1Fu/7xk9TND\n\
TwIDAQABo4IBRjCCAUIwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYw\n\
SwYIKwYBBQUHAQEEPzA9MDsGCCsGAQUFBzAChi9odHRwOi8vYXBwcy5pZGVudHJ1\n\
c3QuY29tL3Jvb3RzL2RzdHJvb3RjYXgzLnA3YzAfBgNVHSMEGDAWgBTEp7Gkeyxx\n\
+tvhS5B1/8QVYIWJEDBUBgNVHSAETTBLMAgGBmeBDAECATA/BgsrBgEEAYLfEwEB\n\
ATAwMC4GCCsGAQUFBwIBFiJodHRwOi8vY3BzLnJvb3QteDEubGV0c2VuY3J5cHQu\n\
b3JnMDwGA1UdHwQ1MDMwMaAvoC2GK2h0dHA6Ly9jcmwuaWRlbnRydXN0LmNvbS9E\n\
U1RST09UQ0FYM0NSTC5jcmwwHQYDVR0OBBYEFHm0WeZ7tuXkAXOACIjIGlj26Ztu\n\
MA0GCSqGSIb3DQEBCwUAA4IBAQAKcwBslm7/DlLQrt2M51oGrS+o44+/yQoDFVDC\n\
5WxCu2+b9LRPwkSICHXM6webFGJueN7sJ7o5XPWioW5WlHAQU7G75K/QosMrAdSW\n\
9MUgNTP52GE24HGNtLi1qoJFlcDyqSMo59ahy2cI2qBDLKobkx/J3vWraV0T9VuG\n\
WCLKTVXkcGdtwlfFRjlBz4pYg1htmf5X6DYO8A4jqv2Il9DjXA6USbW1FzXSLr9O\n\
he8Y4IWS6wY7bCkjCWDcRQJMEhg76fsO3txE+FiYruq9RUWhiF1myv4Q6W+CyBFC\n\
Dfvp7OOGAN6dEOM4+qR9sdjoSYKEBpsr6GtPAQw4dy753ec5\n\
-----END CERTIFICATE-----\n";

using namespace nlohmann;
using namespace nlohmann::detail;

weather_map_ext maps[16];
PNG png_decoder;
MC_23LCV1024 spi_sram(15625000);

#define CONFIG_ADDR 0x00010000
#define DEFAULT_PALETTE 4
#define DEFAULT_SPEED 60
#define MIN_SPEED 15
#define MAX_SPEED 120
#define PALETTE_PIN 15
#define SPEED_PIN 14

const uint32_t* palettes[] = {
    black_and_white_color_table,
    original_color_table,
    universal_blue_color_table,
    titan_color_table,
    weather_channel_color_table,
    meteored_color_table,
    nexrad_level_iii_color_table,
    rainbow_at_selex_si_color_table,
    dark_sky_color_table
};

void dump_bytes(const uint8_t *bptr, uint32_t len) {
    unsigned int i = 0;
    info("dump_bytes %d", len);
    for (i = 0; i < len;) {
        if ((i & 0x0f) == 0) {
            info_cont1("\n");
        } else if ((i & 0x07) == 0) {
            info_cont1(" ");
        }
        info_cont("%02x ", bptr[i++]);
    }
    info_cont1("\n");
}

void netif_status_callback(netif* netif) {
    info1("netif status change:\n");
    info1("    Link ");
    if(netif_is_link_up(netif)) {
        info_cont1("UP\n");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    } else {
        info_cont1("DOWN\n");
        for(int i = 0; i < 3; i++) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(100);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            sleep_ms(250);
        }
    }
    const ip4_addr_t *address = netif_ip4_addr(netif);
    info1("    IP Addr: ");
    if(address) {
        info_cont("%d.%d.%d.%d\n", ip4_addr1(address), ip4_addr2(address), ip4_addr3(address), ip4_addr4(address));
    } else {
        info_cont1("(null)\n");
    }
}

bool parse_weather_maps(json *array, uint64_t* generated, repeating_timer_t* timer) {
    assert(array->type() == value_t::array);
    assert(generated != nullptr);
    http_client client("https://api.rainviewer.com", {(uint8_t*)ISRG_ROOT_X1_CERT, sizeof(ISRG_ROOT_X1_CERT)});
    bool responded = false;

    client.on_response([&client, array, &responded, generated]() {
        const http_response& response = client.response();
        info("Got response: %d %s\n", response.status(), std::string(response.get_status_text()).c_str());
        if(response.status() == 200) {
            json data = json::parse(response.get_body());
            *generated = data["generated"];
            for(auto it = data["radar"]["past"].begin(); it != data["radar"]["past"].end(); it++) {
                array->push_back(*it);
            }
            for(auto it = data["radar"]["nowcast"].begin(); it != data["radar"]["nowcast"].end(); it++) {
                array->push_back(*it);
            }
        }
        responded = true;
    });

    client.header("Connection", "close");
    cancel_repeating_timer(timer);
    client.get("/public/weather-maps.json");
    uint32_t i = 3000;
    while(i > 0 && !responded && !client.has_error()) {
        sleep_ms(10);
        i--;
    }
    add_repeating_timer_us(timer->delay_us, timer->callback, timer->user_data, timer);
    // Wait for the connection to close
    debug1("parse_weather_maps: Waiting for client to disconnect...\n");
    while(client.connected()) {
        sleep_ms(10);
    }
    debug1("parse_weather_maps: Client disconnected.\n");
    uint16_t status = client.response().status();
    debug("parse_weather_maps: Client status %d\n", status);
    return status >= 200 && status < 300;
}

uint8_t lines[4][256];
void write_line(int row, weather_map* scratch) {
    debug("Writing line %d to scratch weather map\n", row);
    for(int col = 0; col < 64; col++) {
        uint16_t sum = 0;
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                sum += lines[i][col * 4 + j];
            }
        }
        scratch->set_pixel(row, col, sum / 16);
    }
}

void png_draw_callback(PNGDRAW *draw) {
    weather_map* scratch = (weather_map*)draw->pUser;
    if(draw->y % 4 == 0 && draw->y != 0) {
        int row = (draw->y / 4) - 1;
        write_line(row, scratch);
    }
    int stride = draw->iPitch / draw->iWidth;
    for(int i = 0; i < draw->iWidth; i++) {
        lines[draw->y % 4][i] = draw->pPixels[stride * i];
    }
}

bool download_full_map(weather_map* scratch, http_client &client, std::string target, bool final, uint32_t map_index) {
    client.header("Connection", final ? "close" : "keep-alive");
    client.get(target);
    uint32_t timeout = 3000;
    while(timeout > 0 && !client.has_response() && !client.has_error()) {
        sleep_ms(10);
        timeout--;
    }
    if(timeout == 0) {
        error1("Timed out!\n");
        return false;
    }
    else if(client.has_error()) {
        error1("Failed to download map!\n");
        return false;
    }
    const http_response& response = client.response();
    if(response.get_body().size() > 0) {
        int rc = png_decoder.openRAM((uint8_t*)response.get_body().data(), response.get_body().size(), png_draw_callback);
        if(rc != PNG_SUCCESS) {
            error("While opening PNG: code %d\n", rc);
            return false;
        }
        info("PNG opened.\n    Width:  %d\n    Height: %d\n    Pixel type: %d\n", png_decoder.getWidth(), png_decoder.getHeight(), png_decoder.getPixelType());
        rc = png_decoder.decode(scratch, 0);
        if(rc != PNG_SUCCESS) {
            error("While decoding PNG: code %d\n", rc);
            return false;
        }
        // Write final line since there isn't a 256th line to trigger it
        write_line(63, scratch);
        info1("Decoded PNG successfully. Saving to SRAM...\n");
        maps[map_index] = scratch->save(map_index * 4096);
        info1("Saved.\n");
    }
    return true;
}

uint8_t download_weather_maps(json& rain_maps, double lat, double lon, uint8_t z, weather_map* scratch, repeating_timer_t* timer) {
    uint8_t start = 0, count = rain_maps.size();
    uint32_t first_timestamp = rain_maps[0]["time"];
    for(uint8_t i = 0; i < count; i++) {
        if((time_t)first_timestamp == maps[i].timestamp()) {
            start = i;
            break;
        }
    }

    json to_download = json::array();

    for(uint8_t i = 0; i < count; i++) {
        uint8_t idx = (start + i) % 16;
        bool nowcast = rain_maps[i]["path"].get<std::string>().find("nowcast") != std::string::npos;
        if(maps[idx].nowcast() || maps[idx].timestamp() != rain_maps[i]["time"].get<time_t>()) {
            maps[idx].set_nowcast(nowcast);
            maps[idx].set_timestamp(rain_maps[i]["time"].get<time_t>());
            rain_maps[i]["idx"] = idx;
            to_download.push_back(rain_maps[i]);
        }
    }
    info("Downloading %d maps...\n%s\n", to_download.size(), to_download.dump(4).c_str());
    http_client client("https://tilecache.rainviewer.com", {(uint8_t*)ISRG_ROOT_X1_CERT, sizeof(ISRG_ROOT_X1_CERT)});

    client.on_response([&client]() {
        const http_response& response = client.response();
        info("Got response: %d %s\n", response.status(), std::string(response.get_status_text()).c_str());
        auto& headers = response.get_headers();
        if(response.status() == 200) {
            info("Received %s of size %s\n", std::string(headers.at("Content-Type")).c_str(), std::string(headers.at("Content-Length")).c_str());
        } else if(response.status() == 206) {
            info("Receiving %s of range %s\n", std::string(headers.at("Content-Type")).c_str(), std::string(headers.at("Content-Range")).c_str());
        }
    });

    int8_t retry = -1;
    for(uint8_t i = 0; i < to_download.size(); i++) {
        if(retry != -1) {
            i = retry;
            retry = -1;
        }
        char buf[64];
        sprintf(buf, "/256/%d/%.4lf/%.4lf/0/0_1.png", z, lat, lon);
        std::string target = to_download[i]["path"].get<std::string>() + buf;
        info("Requesting %s\n", target.c_str());
        cancel_repeating_timer(timer);
        client.head(target);
        uint32_t timeout = 3000;
        while(timeout > 0 && !client.has_response()) {
            sleep_ms(10);
            timeout--;
        }
        if(timeout == 0) {
            error1("Timed out!\n");
            retry = i;
            continue;
        }
        const http_response& response = client.response();
        int length;
        std::string_view content_length = response.get_headers().at("Content-Length");
        std::from_chars(content_length.begin(), content_length.end(), length);
        scratch->load(maps[to_download[i]["idx"].get<uint32_t>()], false);
        if(length < TCP_WND) {
            download_full_map(scratch, client, target, i == to_download.size() - 1, to_download[i]["idx"].get<uint32_t>());
        } else if(response.get_headers().at("Accept-Ranges") == "bytes") {
            // download_chunked_map(scratch, client, target, i == to_download.size() - 1, to_download[i]["idx"].get<uint32_t>(), length);
            info1("Would try partial download here.\n");
        } else {
            info1("Cannot download image, partial download not supported for this image.\n");
        }
        add_repeating_timer_us(timer->delay_us, timer->callback, timer->user_data, timer);
    }
    // Wait for the connection to close
    debug1("download_weather_maps: Waiting for client to disconnect...\n");
    while(client.connected()) {
        sleep_ms(10);
    }
    debug1("download_weather_maps: Client disconnected.\n");
    return start;
}

int64_t update_maps_alarm(alarm_id_t alarm, void* user_data) {
    bool* update_maps = (bool*)user_data;
    *update_maps = true;
    return 0;
}

struct refresh_data {
    rgb_matrix<64, 64>* matrix;
    MC_23LCV1024* sram;
    const uint8_t *start;
    uint8_t display_index;
    uint32_t map_advance;
    uint32_t* map_increment;
};

void redraw_map(rgb_matrix<64, 64>*, MC_23LCV1024*, uint8_t, uint8_t);

bool refresh_display_timer(repeating_timer_t* rt) {
    refresh_data* data = (refresh_data*)rt->user_data;
    trace("Redrawing map...\n    map_advance = %d\n    display_index = %d\n", data->map_advance, data->display_index);
    redraw_map(data->matrix, data->sram, *data->start, data->display_index);

    if(data->map_advance >= 600) {
        data->map_advance = 0;
        data->display_index = (data->display_index + 1) % 16;
    }
    data->map_advance += *data->map_increment;
    trace1("Redrew map.\n");
    return true;
}

struct config_t {
    uint8_t speed;
    uint8_t palette;
    uint16_t padding;
};

config_t config;
volatile uint8_t current_palette = DEFAULT_PALETTE;
volatile uint32_t current_speed = DEFAULT_SPEED;
absolute_time_t last_call = get_absolute_time();
void save_config();
void change_palette_interrupt(uint pin, uint32_t event_mask) {
    // Wait at least 1/4 second between changes
    absolute_time_t prev = last_call, now = get_absolute_time();
    if(absolute_time_diff_us(prev, now) < 250000) {
        return;
    }
    last_call = now;
    if (pin == PALETTE_PIN) {
        current_palette = (current_palette + 1) % (sizeof(palettes) / sizeof(uint32_t*));
        config.palette = current_palette;
    } else if (pin == SPEED_PIN) {
        if(current_speed <= MIN_SPEED) {
            current_speed = MAX_SPEED;
        } else {
            current_speed = current_speed >> 1;
        }
        config.speed = current_speed;
    }
    save_config();
}

uint32_t forward_distance_mod_n(int32_t first, int32_t second, uint32_t n) {
    int32_t distance = (second % n) - (first % n);
    return distance >= 0 ? distance : n + distance;
}

void redraw_map(rgb_matrix<64, 64>* matrix, MC_23LCV1024* sram, uint8_t start, uint8_t display_index) {
    uint32_t index = display_index * 4096;
    uint8_t pixel = 0;
    for(int row = 0; row < 64; row++) {
        for(int col = 0; col < 64; col++) {
            // row * 64 + col
            sram->read(index + row * 64 + col, {&pixel, 1});
            matrix->set_pixel(row, col, palettes[current_palette][pixel]);
        }
    }
    datetime_t datetime;
    rtc_get_datetime(&datetime);
    struct tm time = ntp_client::localtime(datetime);
    char date_str[8];
    char time_str[8];
    char map_time_str[8];
    size_t date_len = std::strftime(date_str, 8, "%m-%d", &time);
    size_t time_len = std::strftime(time_str, 8, "%R", &time);
    struct tm map_time = ntp_client::localtime(maps[display_index].timestamp());
    size_t map_time_len = std::strftime(map_time_str, 8, "%R", &map_time);

    matrix->draw_str(11, 2, 0xFFFFFFFF, {date_str, date_len});
    matrix->draw_str(17, 2, 0xFFFFFFFF, {time_str, time_len});
    matrix->draw_str(23, 2, 0xFFFFFFFF, {map_time_str, map_time_len});
    for(int i = 1; i <= 8; i++) {
        matrix->set_pixel(62, i - 1, palettes[current_palette][16 * i - 1]);
        matrix->set_pixel(63, i - 1, palettes[current_palette][(16 * i - 1) | 0x80]);
    }
    uint32_t distance = forward_distance_mod_n(start, display_index, 16);
    for(int i = 0; i < 16; i++) {
        matrix->set_pixel(63, i + 24, (MAX(255 - 20 * (i + 1), 10)) & 0xFF | (MAX(10, 85 * (i + 1 - 13)) & 0xFF) << 16 | (i == distance ? 0x0000b700 : 0));
    }
    matrix->flip_buffer();
}

void load_config() {
    uint32_t config_crc32, stored_crc32;
    spi_sram.read(CONFIG_ADDR, {(uint8_t*)&config, sizeof(config_t)});
    spi_sram.read(CONFIG_ADDR + sizeof(config_t), {(uint8_t*)&stored_crc32, sizeof(uint32_t)});
    config_crc32 = spi_sram.validate(CONFIG_ADDR, sizeof(config_t));
    if(stored_crc32 != config_crc32) {
        info1("Loading default config\n");
        config.speed = DEFAULT_SPEED;
        config.palette = DEFAULT_PALETTE;
        config.padding = 0xCCAA;
    } else {
        info("Loaded config:\n    speed %d\n    palette %d\n", config.speed, config.palette);
    }
    current_speed = config.speed;
    current_palette = config.palette;
}

void save_config() {
    uint32_t config_crc32 = crc32buf((char*)&config, sizeof(config_t));
    spi_sram.write(CONFIG_ADDR, {(uint8_t*)&config, sizeof(config_t)});
    spi_sram.write(CONFIG_ADDR + sizeof(config_t), {(uint8_t*)&config_crc32, sizeof(uint32_t)});
    info("Saved config:\n    speed %d\n    palette %d\n    crc 0x%08x\n", config.speed, config.palette, config_crc32);
}

int main() {
    stdio_init_all();
    rtc_init();
    setenv("TZ", TIMEZONE, 1);
    tzset();

    gpio_set_dir(PALETTE_PIN, false);
    gpio_set_pulls(PALETTE_PIN, true, false);
    gpio_set_irq_enabled_with_callback(PALETTE_PIN, GPIO_IRQ_EDGE_FALL, true, change_palette_interrupt);

    gpio_set_dir(SPEED_PIN, false);
    gpio_set_pulls(SPEED_PIN, true, false);
    gpio_set_irq_enabled_with_callback(SPEED_PIN, GPIO_IRQ_EDGE_FALL, true, change_palette_interrupt);

    if(cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
        printf("Wi-Fi init failed");
        return -1;
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(2000);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    cyw43_arch_enable_sta_mode();
    info("Connecting to WiFi SSID %s...\n", WIFI_SSID);

    netif_set_status_callback(netif_default, netif_status_callback);

    int err = cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
    int status = CYW43_LINK_UP + 1;

    while(!(netif_is_link_up(netif_default) && netif_ip4_addr(netif_default)->addr != 0)) {
        if(err || status < 0) {
            if(err) {
                error("failed to start wifi scan (code %d).\n", err);
            } else {
                error("failed to join network (code %d).\n", status);
            }
            sleep_ms(1000);
            err = cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
            status = CYW43_LINK_UP + 1;
            continue;
        } 
        int new_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        if(status != new_status) {
            debug("Wifi status change: %d -> %d\n", status, new_status);
            status = new_status;
        }
    }

    ntp_client ntp("pool.ntp.org");
    // Repeat sync once per day at 10am UTC (3am AZ)
    datetime_t repeat = {.year = -1, .month = -1, .day = -1, .dotw = -1, .hour = 10, .min = 0, .sec = 0};
    ntp.sync_time(&repeat);

    load_config();
    weather_map scratch{&spi_sram};
    scratch.clear();
    save_config();

    rgb_matrix<64, 64> *matrix = new rgb_matrix<64, 64>();
    matrix->clear();
    matrix->start();

    bool update_maps = true;
    uint8_t start = 0;
    refresh_data refresh_config;
    refresh_config.matrix = matrix;
    refresh_config.sram = &spi_sram;
    refresh_config.start = &start;
    refresh_config.display_index = 0;
    refresh_config.map_advance = 0;
    refresh_config.map_increment = (uint32_t*)&current_speed;
    repeating_timer_t timer_data;

    bool success = add_repeating_timer_us(-100000, refresh_display_timer, &refresh_config, &timer_data);

    json rain_maps = json::array();
    uint64_t generated_timestamp = 0;
    char buf[128];

    while(1) {
        if(update_maps && ntp.state() == ntp_state::SYNCED) {
            rain_maps.clear();
            uint8_t retries = 2;
            bool success = parse_weather_maps(&rain_maps, &generated_timestamp, &timer_data);
            while(retries > 0 && !success) {
                warn("Retrying /public/weather-maps.json (%d retries left)\n", retries);
                success = parse_weather_maps(&rain_maps, &generated_timestamp, &timer_data);
                retries--;
            }
            if(!success) {
                // Wait 10 seconds, then try again if no request succeeded
                add_alarm_in_ms(10 * 1000, update_maps_alarm, &update_maps, true);
                update_maps = false;
                continue;
            }
            
            info("Rain maps found as:\n%s\n", rain_maps.dump(4).c_str());

            datetime_t datetime;
            rtc_get_datetime(&datetime);
            struct tm time = ntp_client::localtime(datetime);
            uint32_t delay_seconds = 660 - (mktime(&time) % 600);
            time.tm_sec += delay_seconds;
            add_alarm_in_ms(delay_seconds * 1000, update_maps_alarm, &update_maps, true);
            info("Will alarm in %d seconds\n", delay_seconds);

            start = download_weather_maps(rain_maps, LAT, LNG, ZOOM, &scratch, &timer_data);
            update_maps = false;
            time.tm_isdst = -1;

            time_t converted = mktime(&time);
            std::strftime(buf, 128, "%F %T UTC%z", localtime_r(&converted, &time));

            info("Updated maps, sleeping until %s\n", buf);
        }
    }

    return 0;
}