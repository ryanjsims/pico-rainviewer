#define LOG_LEVEL LOG_LEVEL_DEBUG

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

#define ISRG_ROOT_X1_CERT "-----BEGIN CERTIFICATE-----\n\
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
-----END CERTIFICATE-----\n"

using namespace nlohmann;
using namespace nlohmann::detail;

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

void parse_weather_maps(json& array, uint64_t* generated) {
    assert(array.type() == value_t::array);
    assert(generated != nullptr);
    http_client client("https://api.rainviewer.com", {(uint8_t*)ISRG_ROOT_X1_CERT, sizeof(ISRG_ROOT_X1_CERT)});
    bool responded = false;

    client.on_response([&client, &array, &responded, generated]() {
        const http_response& response = client.response();
        info("Got response: %d %s\n", response.status(), response.get_status_text().c_str());
        if(response.status() == 200) {
            json data = json::parse(response.get_body());
            *generated = data["generated"];
            for(auto it = data["radar"]["past"].begin(); it != data["radar"]["past"].end(); it++) {
                array.push_back(*it);
            }
            for(auto it = data["radar"]["nowcast"].begin(); it != data["radar"]["nowcast"].end(); it++) {
                array.push_back(*it);
            }
        }
        responded = true;
    });

    client.header("Connection", "close");
    client.get("/public/weather-maps.json");
    uint32_t i = 50;
    while(i > 0 && !responded) {
        sleep_ms(100);
        i--;
    }
}

int main() {
    stdio_init_all();
    rtc_init();
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

    char buf[256];
    datetime_t datetime;
    rtc_get_datetime(&datetime);
    datetime_to_str(buf, sizeof(buf), &datetime);
    info("Got rtc time:\n%s\n", buf);

    ntp_client ntp("pool.ntp.org");
    ntp.sync_time();

    json rain_maps = json::array();
    uint64_t generated_timestamp = 0;
    parse_weather_maps(rain_maps, &generated_timestamp);

    while(1) {
        datetime_t datetime;
        rtc_get_datetime(&datetime);
        datetime_to_str(buf, sizeof(buf), &datetime);
        printf("%s\r", buf);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(500);
    }
    printf("\nLeft the while loop???\n");

    return 0;
}