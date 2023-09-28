#include <stdio.h>
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
weather_map scratch;
PNG png_decoder;

void dump_bytes(const uint8_t *bptr, uint32_t len) {
    unsigned int i = 0;
    sizeof(weather_map) * 16;
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

bool parse_weather_maps(json *array, uint64_t* generated) {
    assert(array->type() == value_t::array);
    assert(generated != nullptr);
    http_client client("https://api.rainviewer.com", {(uint8_t*)ISRG_ROOT_X1_CERT, sizeof(ISRG_ROOT_X1_CERT)});
    bool responded = false;

    client.on_response([&client, array, &responded, generated]() {
        const http_response& response = client.response();
        info("Got response: %d %s\n", response.status(), response.get_status_text().c_str());
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
    client.get("/public/weather-maps.json");
    uint32_t i = 3000;
    while(i > 0 && !responded) {
        sleep_ms(10);
        i--;
    }
    // Wait for the connection to close
    while(client.connected()) {
        sleep_ms(10);
    }
    uint16_t status = client.response().status();
    return status >= 200 && status < 300;
}

uint8_t lines[4][256];
void write_line(int row) {
    debug("Writing line %d to scratch weather map\n", row);
    for(int col = 0; col < 64; col++) {
        uint16_t sum = 0;
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                sum += lines[i][col * 4 + j];
            }
        }
        scratch.set_pixel(row, col, sum / 16);
    }
}

void png_draw_callback(PNGDRAW *draw) {
    if(draw->y % 4 == 0 && draw->y != 0) {
        int row = (draw->y / 4) - 1;
        write_line(row);
    }
    int stride = draw->iPitch / draw->iWidth;
    for(int i = 0; i < draw->iWidth; i++) {
        lines[draw->y % 4][i] = draw->pPixels[stride * i];
    }
}

uint8_t download_weather_maps(json& rain_maps, double lat, double lon, uint8_t z) {
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
        info("Got response: %d %s\n", response.status(), response.get_status_text().c_str());
        if(response.status() == 200) {
            auto headers = response.get_headers();
            info("Received %s of size %s\n", headers["Content-Type"].c_str(), headers["Content-Length"].c_str());
        }
    });

    int8_t retry = -1;
    for(uint8_t i = 0; i < to_download.size(); i++) {
        if(retry != -1) {
            i = retry;
            retry = -1;
        }
        client.header("Connection", i == to_download.size() - 1 ? "close" : "keep-alive");
        char buf[64];
        sprintf(buf, "/256/%d/%.4lf/%.4lf/0/0_1.png", z, lat, lon);
        info("Requesting %s\n", (to_download[i]["path"].get<std::string>() + buf).c_str());
        client.get(to_download[i]["path"].get<std::string>() + buf);
        uint32_t timeout = 3000;
        while(timeout > 0 && !client.has_response()) {
            sleep_ms(10);
            timeout--;
        }
        if(timeout == 0) {
            error1("Timed out!");
            retry = i;
            continue;
        }
        scratch.load(maps[to_download[i]["idx"].get<uint32_t>()], false);
        const http_response& response = client.response();
        int rc = png_decoder.openRAM((uint8_t*)response.get_body().data(), response.get_body().size(), png_draw_callback);
        if(rc != PNG_SUCCESS) {
            error("While opening PNG: code %d\n", rc);
            continue;
        }
        info("PNG opened.\n    Width:  %d\n    Height: %d\n    Pixel type: %d\n", png_decoder.getWidth(), png_decoder.getHeight(), png_decoder.getPixelType());
        rc = png_decoder.decode(nullptr, 0);
        if(rc != PNG_SUCCESS) {
            error("While decoding PNG: code %d\n", rc);
            continue;
        }
        // Write final line since there isn't a 256th line to trigger it
        write_line(63);
        info1("Decoded PNG successfully. Saving to SRAM...\n");
        maps[to_download[i]["idx"].get<uint32_t>()] = scratch.save(to_download[i]["idx"].get<uint32_t>() * 4096);
        info1("Saved.\n");
    }
    // Wait for the connection to close
    while(client.connected()) {
        sleep_ms(10);
    }
    return start;
}

int64_t update_maps_alarm(alarm_id_t alarm, void* user_data) {
    bool* update_maps = (bool*)user_data;
    *update_maps = true;
    return 0;
}

struct refresh_data {
    int64_t period;
    bool *refresh_display;
};

int64_t refresh_display_alarm(alarm_id_t alarm, void* user_data) {
    refresh_data* data = (refresh_data*)user_data;
    *data->refresh_display = true;
    return -data->period;
}

int main() {
    stdio_init_all();
    rtc_init();
    setenv("TZ", TIMEZONE, 1);
    tzset();

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

    rgb_matrix<64, 64> *matrix = new rgb_matrix<64, 64>();
    matrix->clear();
    matrix->flip_buffer();
    matrix->start();

    bool update_maps = true;
    bool refresh_display = true;
    refresh_data refresh_config;
    refresh_config.refresh_display = &refresh_display;
    refresh_config.period = 2000000;
    alarm_id_t refresh_alarm = -1;

    uint8_t display_index = 0;

    ntp_client ntp("pool.ntp.org");
    // Repeat sync once per day at 10am UTC (3am AZ)
    datetime_t repeat = {.year = -1, .month = -1, .day = -1, .dotw = -1, .hour = 10, .min = 0, .sec = 0};
    ntp.sync_time(&repeat);

    json rain_maps = json::array();
    uint64_t generated_timestamp = 0;
    char buf[128];

    while(1) {
        if(!refresh_display && update_maps && ntp.state() == ntp_state::SYNCED) {
            rain_maps.clear();
            uint8_t retries = 2;
            bool success = parse_weather_maps(&rain_maps, &generated_timestamp);
            while(retries > 0 && !success) {
                warn("Retrying /public/weather-maps.json (%d retries left)\n", retries);
                success = parse_weather_maps(&rain_maps, &generated_timestamp);
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
            uint32_t delay_seconds = 610 - (mktime(&time) % 600);
            time.tm_sec += delay_seconds;
            add_alarm_in_ms(delay_seconds * 1000, update_maps_alarm, &update_maps, true);
            info("Will alarm in %d seconds\n", delay_seconds);

            download_weather_maps(rain_maps, LAT, LNG, ZOOM);
            update_maps = false;
            time.tm_isdst = -1;

            time_t converted = mktime(&time);
            std::strftime(buf, 128, "%F %T UTC%z", localtime_r(&converted, &time));

            info("Updated maps, sleeping until %s\n", buf);
        }

        if(refresh_display && ntp.state() == ntp_state::SYNCED) {
            if(refresh_alarm == -1) {
                refresh_alarm = add_alarm_in_us(refresh_config.period, refresh_display_alarm, &refresh_config, true);
            }
            scratch.load(maps[display_index]);
            display_index = (display_index + 1) % 16;
            for(int row = 0; row < 64; row++) {
                for(int col = 0; col < 64; col++) {
                    matrix->set_pixel(row, col, scratch.get_color(row, col, weather_channel_color_table));
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
            struct tm map_time = ntp_client::localtime(scratch.timestamp());
            size_t map_time_len = std::strftime(map_time_str, 8, "%R", &map_time);

            matrix->draw_str(11, 2, 0xFFFFFFFF, {date_str, date_len});
            matrix->draw_str(17, 2, 0xFFFFFFFF, {time_str, time_len});
            matrix->draw_str(23, 2, 0xFFFFFFFF, {map_time_str, map_time_len});
            matrix->flip_buffer();
            refresh_display = false;
        }
    }

    return 0;
}