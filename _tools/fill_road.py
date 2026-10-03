#!/usr/bin/env python3
"""豆包修复：用邻近草坪纹理填充场地背景图右侧的裸地/马路区域。
用法: python fill_road.py <input.png> <output.png> <road_start_x> <lawn_start_x> <top_y> <bot_y>
产出为 PNG，经 dependency.pak 覆盖加载。不改动 main.pak。
"""
import sys
from PIL import Image

def fill_road(src, dst, road_start, lawn_start, top, bot):
    im = Image.open(src).convert("RGB")
    w, h = im.size
    px = im.load()
    lawn_w = road_start - lawn_start
    for x in range(road_start, w):
        sx = lawn_start + ((x - lawn_start) % lawn_w)
        for y in range(top, bot + 1):
            px[x, y] = px[sx, y]
    im.save(dst, "PNG")
    print(f"{src} -> {dst}: size {w}x{h}, filled x[{road_start}..{w-1}] y[{top}..{bot}]")

if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    road = int(sys.argv[3]); lawn = int(sys.argv[4]); top = int(sys.argv[5]); bot = int(sys.argv[6])
    fill_road(src, dst, road, lawn, top, bot)
