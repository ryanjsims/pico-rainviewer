# Generating a 3d printable height map

If you want to add a height map to the display that the LEDs will shine through, you can use the `generate_terrain.py` script.

Make an account at https://mapbox.com and generate an API token. Their free tier allows you to download 750k map tiles, which this script should not be able to exceed without crashing your computer. Copy `.env.example` to `.env` and add your token to the file.

To generate a height map, use the following command:

```bash
python3 generate_terrain.py -lat (YOUR LATITUDE HERE) -lon (YOUR LONGITUDE HERE) -z (YOUR ZOOM) --width 256 --height 256 -o stls/height_map.stl -d imgs/height_map.png
```

This will generate a height map centered at the latitude and longitude you provide, at the desired (integer) zoom level, with a width and height of 256 samples, fairly low res. To increase the resolution you can increment the zoom and double the width and height each time. These will get pretty large pretty fast, with a 1024x1024 height map STL being about 100 MB. Also, you will get diminishing returns depending on how detailed you can actually print.

When printing, I'd recommend using a transparent plastic or resin so more light shines through.