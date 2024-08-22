#include <stdio.h>
#include <charconv>
#include <pico/cyw43_arch.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
#include <pico/util/datetime.h>
#include <hardware/rtc.h>
#include <http_client.h>
#include <mqtt_client.h>
#include <ntp_client.h>
#include <nlohmann/json.hpp>
#include "logger.h"
#include <PNGdec.h>

#include <lwip/dns.h>

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
-----END CERTIFICATE-----\n\
-----BEGIN CERTIFICATE-----\n\
MIIEfjCCAuagAwIBAgIRAMlWXu0gsAES7UysKtLsddowDQYJKoZIhvcNAQELBQAw\n\
VzEeMBwGA1UEChMVbWtjZXJ0IGRldmVsb3BtZW50IENBMRYwFAYDVQQLDA1yb290\n\
QGZsdW9yaW5lMR0wGwYDVQQDDBRta2NlcnQgcm9vdEBmbHVvcmluZTAeFw0yMzEx\n\
MDgwNzI2MzRaFw0zMzExMDgwNzI2MzRaMFcxHjAcBgNVBAoTFW1rY2VydCBkZXZl\n\
bG9wbWVudCBDQTEWMBQGA1UECwwNcm9vdEBmbHVvcmluZTEdMBsGA1UEAwwUbWtj\n\
ZXJ0IHJvb3RAZmx1b3JpbmUwggGiMA0GCSqGSIb3DQEBAQUAA4IBjwAwggGKAoIB\n\
gQDLNe4Nt98fjefLmKMBjRwn6rIRWlYaT8tAfvJBawb3Y4ZeyfqhOnJ/Pg74Yifa\n\
LKisGZ95YVseC5kNhxChGDFYgTkt0q2sugbuRkTNe1dLr1UQ6i3E6nKQ/T928UPX\n\
S64BqXQXkyji3R/VbRoNdyDvmyxZD2uLIXuo4tv0kK1cHwvoZ+lgBPrfSiuYPa3A\n\
43FBeK9DWrVM31+XyCognz8TUUd0PQPUTuk1xOgDEijQrypPHM33jeZyRv0yPYT3\n\
T1OODx0IQXbeyfRkHmfKsJxtyddRb33U2lW3m7yGBqKQE8rhr/gAA+ymzPMhk58a\n\
MPBtmSNgWAHr16J829Qx4lBtga6NfFVcV3j8OygQxYJNSmGic2FpsEtPfrO/jgWa\n\
hrxIMjGpeKzc8DVMyxTt5wqapNLuDSh18NeiLrQwW60NCc2MPkdn7f/2/sCwb7IU\n\
iM/eSgDo8sB402l+F6l0zNizvNEaVFggU5uJbnRU944PIp1d6CVkhaA7Nv7wHMMM\n\
yCcCAwEAAaNFMEMwDgYDVR0PAQH/BAQDAgIEMBIGA1UdEwEB/wQIMAYBAf8CAQAw\n\
HQYDVR0OBBYEFLCfJAY0MzytgmfMO3sUfibKqS1PMA0GCSqGSIb3DQEBCwUAA4IB\n\
gQCOxrDpm5Lytj7n93bN3yart0IDh6hBQTJoal6nQT3LjKjCBcuuxSAhZJ6HRpZ5\n\
Xx12cGlYsXdV98kTPDGmuB9SCuNEJGENPz7Fkzgkfcz7F5vS1ccth4aY6d2lJwp7\n\
IkYgHQx4rx+UsG9SeS7xohr78qinuwKEapLDYhiq5/dD0iBAuuLo0v0s5eWw3Bj1\n\
rbWqsHQd+jjComDX4npGcZ0ngu4jt3caqFUxhh5XGrcFd26QV9wQ/XBSdhWFuh/I\n\
jGOy6cmgvUFNpTMIJ7PpAZACK4vuEJ+S5PVf011LazkyUhPzvZasOkMx49cbhNnC\n\
kSIqm5U/YuMsWNVrC1HbOZPwJR3YNmLFNlcI84JfMjhRG8Z3Sc9B9UgdnCNkBeJC\n\
sFgJh/Yqc4JSzABFvXTjXQKlvN0NK6H+nCVDJwbIxjOg6254ZIFraWjrzvnRy+/H\n\
fWKHcbjCMI/QjOTtoNj36eD195qBhD1uDS7Tqvl72dxI0Pj4nqnk7j0QJmcViLOT\n\
zAs=\n\
-----END CERTIFICATE-----";

using namespace nlohmann;
using namespace nlohmann::detail;

std::u8string speed_topic_prefix;
std::u8string palette_topic_prefix;
std::u8string display_topic_prefix;
std::u8string temperature_topic_prefix;
std::u8string discovery_topic = u8"/config";
std::u8string command_topic = u8"/set";
std::u8string state_topic = u8"/state";
std::u8string availability_topic;
const std::u8string homeassistant_status_topic = u8"homeassistant/status";
const std::u8string mqtt_username = (const char8_t*)MQTT_USERNAME;
const std::u8string mqtt_password = (const char8_t*)MQTT_PASSWORD;

weather_map_ext maps[16];
PNG png_decoder;
MC_23LCV1024 spi_sram(15625000);
http_client* client;
mqtt::client* mqtt_ptr = nullptr;

#define CONFIG_ADDR 0x00010000
#define DEFAULT_PALETTE 4
#define DEFAULT_SPEED 60
#define MIN_SPEED 15
#define MAX_SPEED 120
#ifndef DEBUG_PROBE
#define SPEED_PIN 14
#define PALETTE_PIN 15
#else
#define SPEED_PIN 20
#define PALETTE_PIN 21
#endif

struct config_t {
    uint8_t speed;
    uint8_t palette;
    uint16_t padding;
};

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

template<typename ... Args>
std::u8string string_format(const std::u8string& format, Args ... args)
{
    int size_s = std::snprintf(nullptr, 0, (const char*)format.c_str(), args...) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ return u8""; }
    auto size = static_cast<size_t>( size_s );
    char8_t buf[size];
    std::snprintf((char*)buf, size, (const char*)format.c_str(), args...);
    return std::u8string(buf, buf + size - 1); // We don't want the '\0' inside
}

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
    int size_s = std::snprintf(nullptr, 0, (const char*)format.c_str(), args...) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ return ""; }
    auto size = static_cast<size_t>( size_s );
    char buf[size];
    std::snprintf((char*)buf, size, (const char*)format.c_str(), args...);
    return std::string(buf, buf + size - 1); // We don't want the '\0' inside
}

// TODO: Add last modified header check - if last modified hasn't changed, delay the update for a minute
bool parse_weather_maps(json *array, uint64_t* generated, repeating_timer_t* timer) {
    assert(array->type() == value_t::array);
    assert(generated != nullptr);
    //http_client client("https://api.rainviewer.com", {(uint8_t*)ISRG_ROOT_X1_CERT, sizeof(ISRG_ROOT_X1_CERT)});
    client->clear_error();
    client->url("https://api.rainviewer.com");
    bool responded = false;

    client->on_response([array, &responded, generated]() {
        const http_response& response = client->response();
        info("Got response: %d %.*s\n", response.status(), response.get_status_text().size(), response.get_status_text().data());
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

    client->header("Connection", "close");
    cancel_repeating_timer(timer);
    client->get("/public/weather-maps.json");
    uint32_t i = 3000;
    debug1("parse_weather_maps: Waiting for client to get response...\n");
    while(i > 0 && !responded && !client->has_error()) {
        sleep_ms(10);
        i--;
    }
    debug("parse_weather_maps:\n    i = %d\n    responded = %d\n    client->has_error = %d\n", i, responded, client->has_error());
    add_repeating_timer_us(timer->delay_us, timer->callback, timer->user_data, timer);
    // Wait for the connection to close
    debug1("parse_weather_maps: Waiting for client to disconnect...\n");
    while(client->connected()) {
        sleep_ms(10);
    }
    debug1("parse_weather_maps: Client disconnected.\n");
    uint16_t status = client->response().status();
    debug("parse_weather_maps: Client status %d\n", status);
    return status >= 200 && status < 300;
}

void update_temperature_state();
bool update_temperature(float* temperature, repeating_timer_t* timer) {
    if(temperature == nullptr) {
        return false;
    }
    client->clear_error();
    client->url("https://weewx.fluorine.local");
    bool responded = false;

    client->on_response([temperature, &responded]() {
        const http_response& response = client->response();
        info("Got response: %d '%.*s'\n", response.status(), response.get_status_text().size(), response.get_status_text().data());
        if(response.status() == 200) {
            json data = json::parse(response.get_body());
            *temperature = data["outdoors"]["fahrenheit"];
        }
        responded = true;
    });

    client->header("Connection", "close");
    cancel_repeating_timer(timer);
    client->get("/api/temperature");
    uint32_t i = 3000;
    while(i > 0 && !responded && !client->has_error()) {
        sleep_ms(10);
        i--;
    }
    add_repeating_timer_us(timer->delay_us, timer->callback, timer->user_data, timer);
    // Wait for the connection to close
    debug1("update_temperature: Waiting for client to disconnect...\n");
    while(client->connected()) {
        sleep_ms(10);
    }
    update_temperature_state();
    debug1("update_temperature: Client disconnected.\n");
    uint16_t status = client->response().status();
    debug("update_temperature: Client status %d\n", status);
    return status >= 200 && status < 300;
}

uint8_t lines[4][256];
void write_line(int row, weather_map* scratch) {
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

bool decode_png(int rc, weather_map* scratch, uint32_t map_index) {
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
    return true;
}

bool download_full_map(weather_map* scratch, std::string target, bool final, uint32_t map_index) {
    client->header("Connection", final ? "close" : "keep-alive");
    client->get(target);
    uint32_t timeout = 3000;
    while(timeout > 0 && !client->has_response() && !client->has_error()) {
        sleep_ms(10);
        timeout--;
    }
    if(timeout == 0) {
        error1("Timed out!\n");
        return false;
    }
    else if(client->has_error()) {
        error1("Failed to download map!\n");
        return false;
    }
    const http_response& response = client->response();
    if(response.get_body().size() > 0) {
        int rc = png_decoder.openRAM((uint8_t*)response.get_body().data(), response.get_body().size(), png_draw_callback);
        return decode_png(rc, scratch, map_index);
    }
    return false;
}

struct spi_file {
    uint32_t base, pos, length;
};


void *open_spi(const char* filename, int32_t* length) {
    spi_sram.read(CONFIG_ADDR + sizeof(config_t) + sizeof(int32_t), {(uint8_t*)length, sizeof(int32_t)});
    spi_file* handle = (spi_file*)malloc(sizeof(spi_file));
    if(handle == nullptr) {
        return nullptr;
    }
    handle->base = CONFIG_ADDR + sizeof(config_t) + sizeof(int32_t) * 2;
    handle->pos = 0;
    handle->length = *length;
    return (void*)handle;
}

void close_spi(void* handle) {
    free(handle);
}

int32_t read_spi(PNGFILE* png, uint8_t* buffer, int32_t length) {
    spi_file* handle = (spi_file*)png->fHandle;
    int32_t bytes_read = length;
    if((handle->length - handle->pos) < length) {
        bytes_read = (handle->length - handle->pos);
    }
    if(length <= 0) {
        return 0;
    }
    bytes_read = spi_sram.read(handle->base + handle->pos, {buffer, (uint32_t)bytes_read});
    handle->pos += bytes_read;
    png->iPos += bytes_read;
    return bytes_read;
}

int32_t seek_spi(PNGFILE *png, int32_t pos) {
    spi_file* handle = (spi_file*)png->fHandle;
    if(pos <= 0) {
        pos = 0;
    } else if(pos > handle->length) {
        pos = handle->length - 1;
    }
    handle->pos = pos;
    png->iPos = pos;
    return pos;
}

bool download_chunked_map(weather_map* scratch, std::string target, bool final, uint32_t map_index, uint32_t length) {
    constexpr uint32_t chunk_size = 2048;
    uint32_t downloaded = 0;
    char buf[32];
    uint8_t timeouts = 3;
    info("Attempting chunked download of image of length %d\n", length);
    while(downloaded < length) {
        client->header("Connection", final && (downloaded + chunk_size > length) ? "close" : "keep-alive");
        sprintf(buf, "bytes=%d-%d", downloaded, MIN(downloaded + chunk_size - 1, length));
        client->header("Range", buf);
        client->get(target);
        uint32_t timeout = 3000;
        while(timeout > 0 && !client->has_response()) {
            sleep_ms(10);
            timeout--;
        }
        if(timeout == 0 && timeouts > 0) {
            error1("Timed out!\n");
            timeouts--;
            continue;
        } else if(timeouts == 0) {
            return false;
        }
        const std::string_view& body = client->response().get_body();
        int32_t written = spi_sram.write(CONFIG_ADDR + sizeof(config_t) + sizeof(int32_t) * 2 + downloaded, {(uint8_t*)body.data(), body.size()});
        downloaded += body.size();
        printf("Downloaded %d / %d\r", downloaded, length);
    }
    printf("\n");
    spi_sram.write(CONFIG_ADDR + sizeof(config_t) + sizeof(int32_t), {(uint8_t*)&downloaded, sizeof(downloaded)});
    if(length > 0) {
        int rc = png_decoder.open(nullptr, open_spi, close_spi, read_spi, seek_spi, png_draw_callback);
        return decode_png(rc, scratch, map_index);
    }
    return false;
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
    //http_client client("https://tilecache.rainviewer.com", {(uint8_t*)ISRG_ROOT_X1_CERT, sizeof(ISRG_ROOT_X1_CERT)});
    client->clear_error();
    client->url("https://tilecache.rainviewer.com");

    client->on_response([]() {
        const http_response& response = client->response();
        info("Got response: %d %.*s\n", response.status(), response.get_status_text().size(), response.get_status_text().data());
        auto& headers = response.get_headers();
        const std::string_view content_type = headers.at("Content-Type");
        const std::string_view content_length = headers.at("Content-Length");
        if(response.status() == 200) {
            info("Received %.*s of size %.*s\n", content_type.size(), content_type.data(), content_length.size(), content_length.data());
        } else if(response.status() == 206) {
            const std::string_view content_range = headers.at("Content-Range");
            info("Receiving %.*s of range %.*s\n", content_type.size(), content_type.data(), content_range.size(), content_range.data());
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
        client->head(target);
        uint32_t timeout = 3000;
        while(timeout > 0 && !client->has_response()) {
            sleep_ms(10);
            timeout--;
        }
        if(timeout == 0) {
            error1("Timed out!\n");
            retry = i;
            continue;
        }
        const http_response& response = client->response();
        int length;
        std::string_view content_length = response.get_headers().at("Content-Length");
        std::from_chars(content_length.begin(), content_length.end(), length);
        scratch->load(maps[to_download[i]["idx"].get<uint32_t>()], false);
        auto header = response.get_headers().find("Accept-Ranges");
        // if(length < (int)(TCP_WND * 0.75)) {
            download_full_map(scratch, target, i == to_download.size() - 1, to_download[i]["idx"].get<uint32_t>());
        // } else if(header != response.get_headers().end() && header->second == "bytes") {
            // download_chunked_map(scratch, target, i == to_download.size() - 1, to_download[i]["idx"].get<uint32_t>(), length);
        // } else {
            // info1("Cannot download image, partial download not supported for this image.\n");
        // }
        add_repeating_timer_us(timer->delay_us, timer->callback, timer->user_data, timer);
    }
    // Wait for the connection to close
    debug1("download_weather_maps: Waiting for client to disconnect...\n");
    while(client->connected()) {
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

int64_t update_temperature_alarm(alarm_id_t alarm, void* user_data) {
    bool* update_temp = (bool*)user_data;
    *update_temp = true;
    return 0;
}

struct refresh_data {
    rgb_matrix<64, 64>* matrix;
    MC_23LCV1024* sram;
    const uint8_t *start;
    uint8_t display_index;
    uint32_t map_advance;
    uint32_t* map_increment;
    float temperature;
    bool enable;
};

void redraw_map(rgb_matrix<64, 64>*, MC_23LCV1024*, uint8_t, uint8_t, float, bool);

bool refresh_display_timer(repeating_timer_t* rt) {
    refresh_data* data = (refresh_data*)rt->user_data;
    //trace("Redrawing map...\n    map_advance = %d\n    display_index = %d\n", data->map_advance, data->display_index);
    redraw_map(data->matrix, data->sram, *data->start, data->display_index, data->temperature, data->enable);

    if(data->map_advance >= 600) {
        data->map_advance = 0;
        data->display_index = (data->display_index + 1) % 16;
    }
    data->map_advance += *data->map_increment;
    //trace1("Redrew map.\n");
    return true;
}

config_t config;
refresh_data refresh_config;
volatile uint8_t current_palette = DEFAULT_PALETTE;
volatile uint32_t current_speed = DEFAULT_SPEED;
absolute_time_t last_call = get_absolute_time();

std::vector<std::string> palette_names = {
    "0 - Grayscale dBZ Values",
    "1 - Original",
    "2 - Universal Blue",
    "3 - TITAN",
    "4 - The Weather Channel",
    "5 - Meteored",
    "6 - NEXRAD Level III",
    "7 - Rainbow @ SELEX-IS",
    "8 - Dark Sky",
};

void update_speed_state() {
    if(!(mqtt_ptr && mqtt_ptr->connected())) {
        return;
    }
    std::string speed = std::to_string(config.speed);
    mqtt::publish_packet::flags_t flags{};
    flags.qos(1);
    flags.retain(true);
    mqtt_ptr->publish(speed_topic_prefix + state_topic, flags, {(u8_t*)speed.data(), speed.size()});
}

void update_palette_state() {
    if(!(mqtt_ptr && mqtt_ptr->connected())) {
        return;
    }
    std::string palette = palette_names[config.palette];
    mqtt::publish_packet::flags_t flags{};
    flags.qos(1);
    flags.retain(true);
    mqtt_ptr->publish(palette_topic_prefix + state_topic, flags, {(u8_t*)palette.data(), palette.size()});
}

void update_display_state() {
    if(!(mqtt_ptr && mqtt_ptr->connected())) {
        return;
    }
    std::string state = refresh_config.enable ? "ON" : "OFF";
    mqtt::publish_packet::flags_t flags{};
    flags.qos(1);
    flags.retain(true);
    mqtt_ptr->publish(display_topic_prefix + state_topic, flags, {(u8_t*)state.data(), state.size()});
}

void update_temperature_state() {
    if(!(mqtt_ptr && mqtt_ptr->connected())) {
        return;
    }
    char buff[16];
    int written = std::snprintf(buff, sizeof(buff), "%.1f", refresh_config.temperature);
    mqtt::publish_packet::flags_t flags{};
    flags.qos(1);
    flags.retain(true);
    mqtt_ptr->publish(temperature_topic_prefix + state_topic, flags, {(u8_t*)buff, (size_t)written});
}

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
    if(pin == SPEED_PIN) {
        update_speed_state();
    } else {
        update_palette_state();
    }
    if(!refresh_config.enable) {
        refresh_config.enable = true;
        update_display_state();
    }
    save_config();
}

uint32_t forward_distance_mod_n(int32_t first, int32_t second, uint32_t n) {
    int32_t distance = (second % n) - (first % n);
    return distance >= 0 ? distance : n + distance;
}

void redraw_map(rgb_matrix<64, 64>* matrix, MC_23LCV1024* sram, uint8_t start, uint8_t display_index, float temperature, bool enable) {
    uint32_t index = display_index * 4096;
    uint8_t pixel = 0;
    for(int row = 0; row < 64; row++) {
        for(int col = 0; col < 64; col++) {
            // row * 64 + col
            if(enable) {
                sram->read(index + row * 64 + col, {&pixel, 1});
            }
            matrix->set_pixel(row, col, palettes[current_palette][pixel]);
        }
    }
    if(enable) {
        datetime_t datetime;
        rtc_get_datetime(&datetime);
        struct tm time = ntp_client::localtime(datetime);
        char date_str[10];
        char time_str[10];
        char map_time_str[10];
        char temperature_str[10];
        size_t date_len = std::strftime(date_str, 8, "%m-%d", &time);
        size_t time_len = std::strftime(time_str, 8, "%R", &time);
        struct tm map_time = ntp_client::localtime(maps[display_index].timestamp());
        size_t map_time_len = std::strftime(map_time_str, 8, "%R", &map_time);
        sprintf(temperature_str, "% 3.1f\xb0""F", temperature);

        matrix->draw_str(11, 2, 0xFFFFFFFF, {date_str, date_len});
        matrix->draw_str(17, 2, 0xFFFFFFFF, {time_str, time_len});
        matrix->draw_str(23, 2, 0xFFFFFFFF, {map_time_str, map_time_len});
        matrix->draw_str(11, 35, 0xFFFFFFFF, {temperature_str, 7});
        for(int i = 1; i <= 8; i++) {
            matrix->set_pixel(62, i - 1, palettes[current_palette][16 * i - 1]);
            matrix->set_pixel(63, i - 1, palettes[current_palette][(16 * i - 1) | 0x80]);
        }
        uint32_t distance = forward_distance_mod_n(start, display_index, 16);
        for(int i = 0; i < 16; i++) {
            matrix->set_pixel(63, i + 24, (MAX(255 - 20 * (i + 1), 10)) & 0xFF | (MAX(10, 85 * (i + 1 - 13)) & 0xFF) << 16 | (i == distance ? 0x0000b700 : 0));
        }
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

void perform_discovery(std::string unique_id) {
    info1("Performing MQTT discovery...\n");
    json discovery;
    discovery["name"] = "Display";
    discovery["~"] = (char*)display_topic_prefix.c_str();
    discovery["avty_t"] = (char*)(availability_topic).c_str();
    discovery["cmd_t"] = (char*)(u8"~" + command_topic).c_str();
    discovery["dev"] = {};
    discovery["dev"]["ids"] = {(char*)unique_id.c_str()};
    discovery["dev"]["name"] = "Rainviewer";
    discovery["dev"]["mf"] = "ryanjsims";
    discovery["dev"]["mdl"] = unique_id;
    discovery["o"] = {};
    discovery["o"]["name"] = "ryanjsims";
    discovery["o"]["sw"] = "1.0";
    discovery["o"]["url"] = "https://github.com/ryanjsims/pico-rainviewer";
    discovery["qos"] = 1;
    discovery["stat_t"] = (char*)(u8"~" + state_topic).c_str();
    discovery["uniq_id"] = unique_id + "-display";

    auto flags = mqtt::publish_packet::flags_t();
    flags.qos(1);

    std::string discovery_string = discovery.dump();
    info("Publishing display discovery\n%.*s\n", discovery_string.size(), discovery_string.data());
    mqtt_ptr->publish(display_topic_prefix + discovery_topic, flags, {(uint8_t*)discovery_string.data(), discovery_string.size()});

    discovery["~"] = (char*)(palette_topic_prefix).c_str();
    discovery["name"] = "Palette";
    discovery["ops"] = palette_names;
    discovery["uniq_id"] = unique_id + "-palette";

    discovery_string = discovery.dump();
    info("Publishing palette discovery\n%.*s\n", discovery_string.size(), discovery_string.data());
    mqtt_ptr->publish(palette_topic_prefix + discovery_topic, flags, {(uint8_t*)discovery_string.data(), discovery_string.size()});

    discovery["~"] = (char*)(speed_topic_prefix).c_str();
    discovery["min"] = 0;
    discovery["max"] = MAX_SPEED;
    discovery["mode"] = "slider";
    discovery["name"] = "Refresh Delay";
    discovery["uniq_id"] = unique_id + "-speed";
    discovery.erase("ops");

    discovery_string = discovery.dump();
    info("Publishing speed discovery\n%.*s\n", discovery_string.size(), discovery_string.data());
    mqtt_ptr->publish(speed_topic_prefix + discovery_topic, flags, {(uint8_t*)discovery_string.data(), discovery_string.size()});
    
    discovery["~"] = (char*)(temperature_topic_prefix).c_str();
    discovery["min"] = -150;
    discovery["max"] = 150;
    discovery["mode"] = "box";
    discovery["name"] = "Temperature";
    discovery["step"] = 0.1;
    discovery["unit_of_meas"] = "°F";
    discovery["uniq_id"] = unique_id + "-temperature";

    discovery_string = discovery.dump();
    info("Publishing temperature discovery\n%.*s\n", discovery_string.size(), discovery_string.data());
    mqtt_ptr->publish(temperature_topic_prefix + discovery_topic, flags, {(uint8_t*)discovery_string.data(), discovery_string.size()});

    update_display_state();
    update_palette_state();
    update_speed_state();
    update_temperature_state();

    std::string available = "online";
    flags.retain(true);
    mqtt_ptr->publish(availability_topic, flags, {(uint8_t*)available.data(), available.size()});
    info1("MQTT discovery publishes complete\n");
}

int main() {
    stdio_init_all();
    rtc_init();
    setenv("TZ", TIMEZONE, 1);
    tzset();
    dns_init();

    char board_id[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    pico_get_unique_board_id_string(board_id, sizeof(board_id));

    std::u8string u8unique_id = string_format(u8"RV-%.*s", 4, board_id + (sizeof(board_id) - 5));
    std::string unique_id = {(char*)u8unique_id.data(), (char*)u8unique_id.data() + u8unique_id.size()};

    speed_topic_prefix = u8"homeassistant/number/" + u8unique_id + u8"/frequency";
    palette_topic_prefix = u8"homeassistant/select/" + u8unique_id + u8"/palette";
    display_topic_prefix = u8"homeassistant/switch/" + u8unique_id + u8"/display";
    temperature_topic_prefix = u8"homeassistant/number/" + u8unique_id + u8"/temperature";
    availability_topic = u8"homeassistant/device/" + u8unique_id + u8"/avail";
    std::u8string command_topic_filter = u8"homeassistant/+/" + u8unique_id + u8"/+/set";

    std::vector<std::u8string> command_topics = {
        display_topic_prefix + command_topic,
        speed_topic_prefix + command_topic,
        palette_topic_prefix + command_topic,
        temperature_topic_prefix + command_topic,
    };

    gpio_set_dir(PALETTE_PIN, false);
    gpio_set_pulls(PALETTE_PIN, true, false);
    gpio_set_irq_enabled_with_callback(PALETTE_PIN, GPIO_IRQ_EDGE_FALL, true, change_palette_interrupt);

    gpio_set_dir(SPEED_PIN, false);
    gpio_set_pulls(SPEED_PIN, true, false);
    gpio_set_irq_enabled_with_callback(SPEED_PIN, GPIO_IRQ_EDGE_FALL, true, change_palette_interrupt);

    if(cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
        error1("Wi-Fi init failed\n");
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

    http_client http("https://api.rainviewer.com", {(uint8_t*)ISRG_ROOT_X1_CERT, sizeof(ISRG_ROOT_X1_CERT)});
    client = &http;

    ip_addr_t address;
    IP4_ADDR(&address, 192, 168, 0, 2);
    info1("Setting local dns host...");
    err_t rc = dns_local_addhost("weewx.fluorine.local", &address);
    info_cont(" rc = %d\n", rc);

    IP4_ADDR(&address, 192, 168, 0, 1);
    info1("Setting dns servers...\n");
    dns_setserver(0, &address);
    IP4_ADDR(&address, 1, 1, 1, 1);
    dns_setserver(1, &address);

    bool update_maps = true, update_temp = true;
    uint8_t start = 0;

    refresh_config.matrix = matrix;
    refresh_config.sram = &spi_sram;
    refresh_config.start = &start;
    refresh_config.display_index = 0;
    refresh_config.map_advance = 0;
    refresh_config.map_increment = (uint32_t*)&current_speed;
    refresh_config.temperature = 0.0f;
    refresh_config.enable = true;
    repeating_timer_t timer_data;

    bool success = add_repeating_timer_us(-100000, refresh_display_timer, &refresh_config, &timer_data);

    json rain_maps = json::array();
    uint64_t generated_timestamp = 0;
    char buf[128];

    bool mqtt_has_connected = false, mqtt_connecting = false;
    mqtt::client mqtt_client("mqtts://homeassistant.local", {(uint8_t*)ISRG_ROOT_X1_CERT, sizeof(ISRG_ROOT_X1_CERT)});
    mqtt_ptr = &mqtt_client;
    mqtt::publish_handler_t mqtt_sub_handler = [&command_topics, &mqtt_client](std::u8string_view topic, const mqtt::properties& props, std::span<uint8_t> data) {
        std::string_view payload{(char*)data.data(), data.size()};
        mqtt::publish_packet::flags_t flags;
        flags.qos(1);
        if(topic == command_topics[0]) {
            refresh_config.enable = (payload == "ON");
            update_display_state();
        } else if(topic == command_topics[1]) {
            uint8_t value;
            auto result = std::from_chars(payload.begin(), payload.end(), value);
            if(result.ec != std::errc()) {
                return mqtt::reason_code::ERROR_IMPL_SPECIFIC;
            }
            config.speed = std::min(value, (uint8_t)MAX_SPEED);
            current_speed = config.speed;
            save_config();
            update_speed_state();
        } else if(topic == command_topics[2]) {
            uint8_t value;
            auto result = std::from_chars(payload.begin(), payload.end(), value);
            if(result.ec != std::errc()) {
                return mqtt::reason_code::ERROR_IMPL_SPECIFIC;
            }
            config.palette = value % (sizeof(palettes) / sizeof(uint32_t*));
            current_palette = config.palette;
            save_config();
            update_palette_state();
        } else if(topic == command_topics[3]) {
            std::string float_string{payload.begin(), payload.end()};
            float value = std::stof(float_string);
            refresh_config.temperature = value;
            update_temperature_state();
        }
        return mqtt::reason_code::SUCCESS;
    };

    mqtt::publish_handler_t mqtt_disc_handler = [&mqtt_client, &unique_id](std::u8string_view topic, const mqtt::properties& props, std::span<uint8_t> data) {
        std::string_view message = {(char*)data.data(), data.size()};
        if(message == "online") {
            perform_discovery(unique_id);
        }
        return mqtt::reason_code::SUCCESS;
    };

    std::string will_payload = "offline";
    
    mqtt_client.connect(
        mqtt_username,
        {(uint8_t*)mqtt_password.data(), mqtt_password.size()},
        15, availability_topic,
        {(uint8_t*)will_payload.data(), will_payload.size()},
        0, true, {}, {}
    );
    mqtt_client.on_connect([&mqtt_client, &unique_id, &command_topic_filter, &mqtt_has_connected, &mqtt_sub_handler, &mqtt_disc_handler]() {
        mqtt_has_connected = true;
        perform_discovery(unique_id);

        auto options = mqtt::subscribe_packet::options_t{2, false, false, 0};
        mqtt_client.subscribe(command_topic_filter, options, mqtt_sub_handler);
        mqtt_client.subscribe(homeassistant_status_topic, options, mqtt_disc_handler);
    });

    while(1) {
        if(update_maps && ntp.state() == ntp_state::SYNCED && mqtt_client.connected()) {
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
        if(update_temp && ntp.state() == ntp_state::SYNCED && mqtt_client.connected()) {
            info1("Updating temperature\n");
            update_temperature(&refresh_config.temperature, &timer_data);
            update_temp = false;
            // Update every 5 minutes
            add_alarm_in_ms(300000, update_temperature_alarm, &update_temp, true);
        }
        if(!mqtt_client.connected() && mqtt_has_connected && ntp.state() == ntp_state::SYNCED) {
            mqtt_client.connect(mqtt_username, mqtt_password);
            mqtt_has_connected = false;
        }
    }

    return 0;
}