import numpy as np
from stl import mesh
import png
import math
import sys
import requests
import os
from PIL import Image
from argparse import ArgumentParser
from dotenv import load_dotenv
from typing import Tuple
from io import BytesIO

load_dotenv()

token = os.getenv("MAPBOX_TOKEN")

zoom2res = [156543.00, 78271.52, 39135.76, 19567.88, 9783.94,
        4891.97, 2445.98, 1222.99, 611.4962, 305.7481, 152.8741,
        76.437, 38.2185, 19.1093, 9.5546, 4.7773, 2.3887, 1.1943,
        0.5972, 0.2986, 0.1493, 0.0746, 0.0373, 0.0187]

def deg2num(lat_deg, lon_deg, zoom):
    assert 0 <= zoom <= 22, "Use a zoom level between 0 and 22, inclusive"
    lat_rad = math.radians(lat_deg)
    n = 2.0 ** zoom
    xtile = (lon_deg + 180.0) / 360.0 * n
    ytile = (1.0 - math.log(math.tan(lat_rad) + (1 / math.cos(lat_rad))) / math.pi) / 2.0 * n
    return (int(xtile), int(ytile), xtile - int(xtile), ytile - int(ytile))

def png2pixels(pngfile):
    rd = png.Reader(file=pngfile)
    width, height, rows, info = rd.read()
    pixels = []
    if info["alpha"]:
        stride = 4
    else:
        stride = 3
    for row in rows:
        pixels.append(list(zip(row[::stride], row[1::stride], row[2::stride])))
    return pixels

def pixels2heights(pixels):
    heights = []
    for row in pixels:
        heights.append([(int(round(-10000 + ((R * 256 * 256 + G * 256 + B) * 0.1)))) for R,G,B in row])
    return list(reversed(heights))

def coords2idx(row, col, rowlen):
    return col + row * rowlen

def usage():
    return\
"""Usage:
    python3 generate_terrain.py [-i <input_file> -o <output_file> -z <zoom_level> [--z-offset-scale <z_offset_scale>]
                                | -d <lat> <long> <zoom_level>
                                | -h]"""

def url(x, y, z):
    return "https://api.mapbox.com/v4/mapbox.terrain-rgb/{}/{}/{}.pngraw?access_token={}".format(z, x, y, token)

def download(lat: float, lon: float, z: int, dim: Tuple[int, int], name: str):
    to_download = []
    x, y, xrem, yrem = deg2num(lat, lon, z)
    centerx = int(256 * xrem)
    centery = int(256 * yrem)
    xpix, ypix = dim
    bounds = [math.floor((centerx - xpix / 2) / 256),
              math.floor((centery - ypix / 2) / 256),
              math.floor((centerx + xpix / 2) / 256),
              math.floor((centery + ypix / 2) / 256)]
    offsets = (
        int((centerx + abs(bounds[0]) * 256) - xpix / 2),
        int((centery + abs(bounds[1]) * 256) - ypix / 2),
        int((centerx + abs(bounds[0]) * 256) + xpix / 2),
        int((centery + abs(bounds[1]) * 256) + ypix / 2),
    )
    width = (bounds[2] - bounds[0] + 1) * 256
    height = (bounds[3] - bounds[1] + 1) * 256
    images = []
    for i in range(bounds[0], bounds[2] + 1):
        for j in range(bounds[1], bounds[3] + 1):
            to_download.append((x + i, y + j))
        images.append([])
    print("Downloading", end='', flush=True)
    for coords in to_download:
        resp = requests.get(url(coords[0], coords[1], z))
        f = BytesIO(resp.content)
        images[coords[0] - x - bounds[0]].append(Image.open(f))
        print(".", end='', flush=True)
    print("\nDone! Constructing file...")
    result = Image.new("RGBA", (width, height))
    for i, row in enumerate(images):
        for j, tile in enumerate(row):
            tile: Image.Image
            result.paste(tile, (256 * i, 256 * j))
    result.crop(offsets).save(name)
    print("Done!")


def main():
    parser = ArgumentParser()
    parser.add_argument("-i", "--input-file", type=str, help="The input heightmap to convert to stl")
    parser.add_argument("-o", "--output-file", type=str, help="The output stl location")
    parser.add_argument("-z", "--zoom", type=int, required=True, help="The zoom level of the heightmap")
    parser.add_argument("-m", "--minimum-height", type=float, help="The height at which to place the bottom of the stl. Defaults to min height - 2 pixel's meters")
    parser.add_argument("-s", "--stats", action="store_true", help="Display stats about the heightmap and exit")
    parser.add_argument("-lat", "--latitude", type=float, help="Set the latitude of the heightmap to download")
    parser.add_argument("-lon", "--longitude", type=float, help="Set the longitude of the heightmap to download")
    parser.add_argument("-x", "--width", type=int, help="Width of the heightmap to download, in pixels")
    parser.add_argument("-y", "--height", type=int, help="Height of the heightmap to download, in pixels")
    parser.add_argument("-d", "--download-name", type=str, help="File name for the downloaded heightmap")
    args = parser.parse_args()
    z_offset_scale = 2
    hMin = 10000
    if args.latitude is not None and args.longitude is not None and (args.width is None or args.height is None):
        x, y, _, _ = deg2num(args.latitude, args.longitude, args.zoom)
        print(url(x, y, args.zoom))
        return
    elif args.latitude is not None and args.longitude is not None:
        name = args.download_name if args.download_name is not None else "imgs/temp.png"
        download(args.latitude, args.longitude, args.zoom, (args.width, args.height), name)
        if args.output_file is None:
            return
        args.input_file = name

    try:
        print("Opening {}...".format(args.input_file))
        mapfile = open(args.input_file, "rb")
    except OSError:
        if args.input_file == "":
            print(usage(), file=sys.stderr)
        else:
            print("Could not find file '{}'".format(args.input_file), file=sys.stderr)
        sys.exit(1)

    pixels = png2pixels(mapfile)
    heights = pixels2heights(pixels)
    mapfile.close()
    hMax = 0
    for row in heights:
        hMin = min(hMin, min(row))
        hMax = max(hMax, max(row))
    if args.minimum_height is None:
        minz = hMin - z_offset_scale * zoom2res[args.zoom]
    else:
        minz = args.minimum_height
    latitude_scale =  1 if args.latitude is None else math.cos(math.radians(args.latitude))
    if args.stats:
        print("Min height: {} meters".format(hMin))
        print("Max height: {} meters".format(hMax))
        print("Width: {} meters".format(len(heights[0]) * zoom2res[args.zoom] * latitude_scale))
        print("Length: {} meters".format(len(heights) * zoom2res[args.zoom] * latitude_scale))
        print("Zoom: {} ({} meters per pixel)".format(args.zoom, zoom2res[args.zoom] * latitude_scale))
        return 0
    print("Generating vertices...")
    vertices = []
    #generate vertices for top surface
    
    for row in range(len(heights)):
        for col in range(len(heights[row])):
            vertices.append([col * zoom2res[args.zoom] * latitude_scale, row * zoom2res[args.zoom] * latitude_scale, heights[row][col]])

    #generate vertices for west surface
    for row in range(len(heights)):
        vertices.append([0, row * zoom2res[args.zoom] * latitude_scale, minz])

    #generate vertices for south surface
    row = len(heights) - 1
    for col in range(len(heights[row])):
        vertices.append([col * zoom2res[args.zoom] * latitude_scale, row * zoom2res[args.zoom] * latitude_scale, minz])

    #generate vertices for east surface
    col = len(heights[row]) - 1
    for row in range(len(heights) - 2, -1, -1):
        vertices.append([col * zoom2res[args.zoom] * latitude_scale, row * zoom2res[args.zoom] * latitude_scale, minz])

    #generate vertices for north surface
    for col in range(len(heights[0]) - 2, 0, -1):
        vertices.append([col * zoom2res[args.zoom] * latitude_scale, 0, minz])

    #place vertices into array
    vertices = np.array(vertices)
    print("Generated {} vertices!\nGenerating faces...".format(len(vertices)))
    faces = []
    height = len(heights)
    width = len(heights[0])
    #generate faces for top surface
    for row in range(len(heights) - 1):
        for col in range(len(heights[0]) - 1):
            faces.append([coords2idx(row, col, len(heights[row])),
                coords2idx(row, col + 1, len(heights[row])),
                coords2idx(row + 1, col, len(heights[row]))])
            faces.append([coords2idx(row + 1, col, len(heights[row])),
                coords2idx(row, col + 1, len(heights[row])),
                coords2idx(row + 1, col + 1, len(heights[row]))])
    #generate faces for west surface
    col = 0
    for row in range(height - 1):
        faces.append([coords2idx(row, col, width),
            height * width + row + col,
            height * width + row + col + 1])
        faces.append([coords2idx(row + 1, col, width),
            coords2idx(row, col, width),
            height * width + row + col + 1])
    #generate faces for north surface
    row = height - 1
    for col in range(width - 1):
        faces.append([coords2idx(row, col, width),
            coords2idx(row, col + 1, width),
            height * width + row + col + 1])
        faces.append([coords2idx(row, col, width),
            height * width + row + col,
            height * width + row + col + 1])
    #generate faces for east surface
    col = width - 1
    for row in range(height - 1, 0, -1):
        faces.append([coords2idx(row, col, width),
            height * width + (height - 1) + (width - 1) + (height - 1 - row) + (width - 1 - col),
            height * width + (height - 1) + (width - 1) + (height - 1 - row) + (width - 1 - col) + 1])
        faces.append([coords2idx(row - 1, col, width),
            coords2idx(row, col, width),
            height * width + (height - 1) + (width - 1) + (height - 1 - row) + (width - 1 - col) + 1])
    #generate faces for south surface
    row = 0
    for col in range(width - 2, 0, -1):
        faces.append([coords2idx(row, col + 1, width),
            height * width + (height - 1) + (width - 1) + (height - 1 - row) + (width - 1 - col),
            coords2idx(row, col, width)])
        faces.append([coords2idx(row, col, width),
            height * width + (height - 1) + (width - 1) + (height - 1 - row) + (width - 1 - col) + 1,
            height * width + (height - 1) + (width - 1) + (height - 1 - row) + (width - 1 - col)])
    #Special case since bottom vertex at 0, 0 is not duplicated
    faces.append([coords2idx(0, 1, width),
        height * width + row + 0,
        height * width + (height - 1) + (width - 1) + (height - 1 - row) + (width - 1 - 1)])
    faces.append([coords2idx(0, 0, width),
        height * width + row + 0,
        coords2idx(0, 1, width)])

    #generate faces for bottom surface
    faces.append([height * width + 0 + 0,
        height * width + (height - 1) + (width - 1)  + 1,
        height * width + (height - 1)])
    faces.append([height * width + (height - 1) + (width - 1) + 1,
        height * width,
        height * width + (height - 1) + (width - 1) + (height - 1) + 1])

    faces = np.array(faces)
    print("Generated {} faces!\nGenerating mesh...".format(len(faces)))

    terrain_mesh = mesh.Mesh(np.zeros(faces.shape[0], dtype=mesh.Mesh.dtype))
    for i, f in enumerate(faces):
        for j in range(3):
            terrain_mesh.vectors[i][j] = vertices[f[j],:]

    print("Mesh generated, saving to {}...".format(args.output_file))
    terrain_mesh.save(args.output_file)
    print("Saved mesh successfully!")

if __name__ == "__main__":
    main()