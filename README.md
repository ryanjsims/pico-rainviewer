# Pico Rainviewer

This is a Raspberry Pi Pico W project that is meant to be similar in behavior to my [Python weathermap](https://github.com/ryanjsims/weathermap/) project. These projects display the current weathermap, retrieved from the [Rainviewer API](https://www.rainviewer.com/api.html), on a 64x64 RGB LED matrix, like [this one from Adafruit](https://www.adafruit.com/product/4732) or [this one from aliexpress](https://www.aliexpress.us/item/3256804802147561.html).

This Pico W version uses the following libraries:
 * [pico-sdk](https://github.com/raspberrypi/pico-sdk)
 * My [pico-web-client](https://github.com/ryanjsims/pico-web-client) library for mqtt and https requests
    * This library also uses [nlohmann/json](https://github.com/nlohmann/json) to parse downloaded json documents
 * The [PNGdec library](https://github.com/bitbank2/PNGdec) for decoding downloaded PNGs

## Bill of Materials
- Raspberry Pi Pico W (1)
- 64x64 RGB LED Matrix with HUB75E interface (1); with included
  - 8x2 IDC ribbon cable (1)
  - 4 pin power cable (VHR-4N) (1)
- 8x2 IDC socket (1)
- JST S4P-VH socket (1)
- PJ-102AH barrel jack socket (1)
- 5V 4A 5.5mm center positive barrel jack power supply (1)
- 23LCV1024 SPI SRAM (or equivalent 64+KByte SPI memory) (1)
- 6x6 mm tactile push button switches (3) 
    - adjust map animation frequency
    - adjust map color palette
    - reset microcontroller
- 10k ohm through hole resistor (1)
- Custom PCB (1) (Design files TBA to repository)

## Building

Prerequisites are the same as the pico-sdk's, make sure you have the `arm-none-eabi-gcc` compiler installed

```bash
git clone https://github.com/ryanjsims/pico-rainviewer
# Don't recurse submodules cause there are a _lot_ of submodules in the pico-sdk's dependencies
cd pico-rainviewer && git submodule update --init && cd lib/pico-sdk && git submodule update --init && cd ../pico-web-client && git submodule update --init lib/json && cd ../..
mkdir build && cd build
BOARD=pico_w WIFI_SSID=<your-ssid> WIFI_PASSWORD=<your-password> LAT=38.8951 LNG=-77.0364 ZOOM=<zoom> TIMEZONE="EST+5EDT,M3.2.0/2,M11.1.0/2" LOG_LEVEL=LOG_LEVEL_INFO cmake ..
make
```

Alternatively you can build with VSCode using the CMake Tools extension. You still need to init the correct submodules as above, but instead of setting the environment variables on the command line, you can add them to the `.vscode/settings.json` file:
```jsonc
// .vscode/settings.json
{
    "cmake.environment": {
        "PICO_BOARD": "pico_w",
        "WIFI_SSID": "<your ssid>",
        "WIFI_PASSWORD": "<your password>",
        "LAT": 38.8951, // Washington DC, set to your desired display location
        "LNG": -77.0364,
        "ZOOM": 7,
        "TIMEZONE": "EST+5EDT,M3.2.0/2,M11.1.0/2", // see https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
        "LOG_LEVEL": "LOG_LEVEL_INFO" // See pico-web-client/include/logger.h for available levels
    }
}
```

After configuring the project with VSCode/CMake Tools, you'll be able to build the project using Ctrl-Shift-B or the build button. Program the Pico W as normal by copying the `pico-rainviewer.uf2` file onto the Pico after resetting it with the bootsel button held down.

Note: The RGB matrix code currently has issues when compiling in release mode - it probably needs to have some variables marked as volatile or something somewhere. To make sure the current iteration of the code works you'll need to be sure to be compiling in debug mode.