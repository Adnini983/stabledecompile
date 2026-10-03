#!/usr/bin/env python3
"""豆包修复：填充泳池/浓雾背景右侧暴露的街道(阴影)区域为草地。
泳池背景中部有空白矩形(泳池，由 PoolEffect 覆盖)，因此填充源在泳池
纵向范围(bgY~330-520)内改用上方草坪的草地质感，其余行沿用同y草坪。
用法: python fill_pool_grass.py <input> <output> <road_start_x> <pool_y0> <pool_y1>
"""
import sys, numpy as np, cv2

def fill_pool_grass(src, dst, road_start, pool_y0, pool_y1):
    img = cv2.imread(src)                     # BGR
    h, w = img.shape[:2]
    lawn_src_x0, lawn_src_x1 = 700, 1100      # 草坪源区间
    srcw = lawn_src_x1 - lawn_src_x0
    # 上方纯草坪草地质感带(作为泳池纵向范围内的源)
    top_band = img[200:330, lawn_src_x0:lawn_src_x1].copy()
    bh = top_band.shape[0]
    top = 130
    for x in range(road_start, w):
        for y in range(top, h):
            if pool_y0 <= y < pool_y1:
                sy = 200 + ((y - pool_y0) % bh)
                sx = lawn_src_x0 + ((x - road_start) % srcw)
            else:
                sy = y
                sx = lawn_src_x0 + ((x - road_start) % srcw)
            img[y, x] = img[sy, sx]
    # 边界羽化融合 road_start 附近(30px)
    blend = 30
    for x in range(road_start - blend, road_start):
        if x < 0: continue
        t = (x - (road_start - blend)) / float(blend)
        for y in range(top, h):
            a = img[y, x].astype(np.float32)
            b = img[y, x + blend].astype(np.float32) if x+blend < w else a
            img[y, x] = (a*(1-t) + b*t).astype(np.uint8)
    cv2.imwrite(dst, img)
    print(f"{src} -> {dst}: road[{road_start}..{w-1}] y[{top}..{h-1}] pool[{pool_y0}..{pool_y1}]")

if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    road, py0, py1 = [int(v) for v in sys.argv[3:6]]
    fill_pool_grass(src, dst, road, py0, py1)
