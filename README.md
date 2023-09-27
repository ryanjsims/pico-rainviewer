# Pico Rainviewer

This is a Raspberry Pi Pico W project that is meant to be similar in behavior to my [Python weathermap](https://github.com/ryanjsims/weathermap/) project. These projects display the current weathermap, retrieved from the [Rainviewer API](https://www.rainviewer.com/api.html), on a 64x64 RGB LED matrix, like [this one from Adafruit](https://www.adafruit.com/product/3649) (out of stock at the time of writing 9/24/23, though other sizes are available).

This Pico W version uses the C/C++ [pico-sdk](https://github.com/raspberrypi/pico-sdk) and my [pico-web-client](https://github.com/ryanjsims/pico-web-client) library to make HTTP requests and download files. I plan to use the [PNGdec library](https://github.com/bitbank2/PNGdec) to decode downloaded PNGs.

## Building

Prerequisites are the same as the pico-sdk's, make sure you have the `arm-none-eabi-gcc` compiler installed

```bash
git clone https://github.com/ryanjsims/pico-rainviewer
# Don't recurse submodules cause there are a _lot_ of submodules in the pico-sdk's dependencies
cd pico-rainviewer && git submodule update --init && cd lib/pico-sdk && git submodule update --init && cd ../pico-web-client && git submodule update --init lib/json && cd ../..
mkdir build && cd build
BOARD=pico_w WIFI_SSID=<your-ssid> WIFI_PASSWORD=<your-password> LAT=38.8951 LNG=-77.0364 cmake ..
make
```

Alternatively you can build with VSCode using the CMake Tools extension. You still need to init the correct submodules as above, but instead of setting the `WIFI_SSID` and `WIFI_PASSWORD` environment variables on the command line, you can add them to the `.vscode/settings.json` file as such (sans comments):
```jsonc
// .vscode/settings.json
{
    "cmake.environment": {
        "BOARD": "pico_w",
        "WIFI_SSID": "<your ssid>",
        "WIFI_PASSWORD": "<your password>",
        "LAT": 38.8951, // Washington DC, set to your desired display location
        "LNG": -77.0364,
        "TIMEZONE": "EST+5EST,M3.2.0/2,M11.1.0/2" // see https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
    }
}
```

After configuring the project with VSCode/CMake Tools, you'll be able to build the project using Ctrl-Shift-B or the build button. Program the Pico W as normal by copying the `pico-rainviewer.uf2` file onto the Pico after resetting it with the bootsel button held down.