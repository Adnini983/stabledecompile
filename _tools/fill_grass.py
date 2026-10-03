#!/usr/bin/env python3
"""豆包修复：用草坪草地质感自然填充场地背景的裸地（马路）区域。
方法：从草坪中取一块较大草地作为纹理源，平铺到马路区域，并在边界做羽化融合，
避免"整块草坪被复制"的重复痕迹。产出 PNG。
用法: python fill_grass.py <input> <output> <road_start_x> <src_x0> <src_x1> <top_y> <bot_y>
"""
import sys, numpy as np, cv2

def fill_grass(src, dst, road_start, sx0, sx1, top, bot):
    img = cv2.imread(src)
    h, w = img.shape[:2]
    sx0 = max(0, min(w, sx0)); sx1 = max(0, min(w, sx1))
    top = max(0, min(h, top)); bot = max(0, min(h, bot))
    srcw = sx1 - sx0
    # tile source region horizontally to fill road
    for x in range(road_start, w):
        sx = sx0 + ((x - road_start) % srcw)
        for y in range(top, bot+1):
            img[y, x] = img[y, sx]
    # feather blend the seam at road_start boundary (30px)
    blend = 30
    for x in range(road_start - blend, road_start):
        if x < 0: continue
        t = (x - (road_start - blend)) / float(blend)
        for y in range(top, bot+1):
            a = img[y, x].astype(np.float32)
            b = img[y, x + blend].astype(np.float32) if x+blend < w else a
            img[y, x] = (a*(1-t) + b*t).astype(np.uint8)
    cv2.imwrite(dst, img)
    print(f"{src} -> {dst}: road[{road_start}..{w-1}] src[{sx0}..{sx1}] y[{top}..{bot}]")

if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    road, sx0, sx1, top, bot = [int(v) for v in sys.argv[3:8]]
    fill_grass(src, dst, road, sx0, sx1, top, bot)
