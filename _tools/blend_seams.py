#!/usr/bin/env python3
"""豆包修复：对填充区域内部平铺接缝做高斯模糊，消除重复纹理跳变。
用法: python blend_seams.py <image> <road_start_x> <srcw> <top_y> <bot_y>
"""
import sys, numpy as np, cv2

def blend_seams(path, road_start, srcw, top, bot):
    img = cv2.imread(path)
    h, w = img.shape[:2]
    band = 28
    seams = []
    x = road_start
    while x < w:
        seams.append(x); x += srcw
    for sx in seams:
        if sx - band < road_start: 
            # first seam: only right band blur
            x0, x1 = sx, sx + band
        else:
            x0, x1 = sx - band, sx + band
        x0 = max(x0, road_start); x1 = min(x1, w-1)
        if x1 <= x0: continue
        y0, y1 = max(top,0), min(bot, h-1)
        if y1 <= y0: continue
        bandimg = img[y0:y1, x0:x1]
        blurred = cv2.GaussianBlur(bandimg, (0,0), sigmaX=3.0)
        # blend: outer edges keep original, center gets blurred
        mask = np.zeros_like(bandimg, dtype=np.float32)
        mask[:] = 1.0
        bw = x1-x0
        ramp = np.linspace(0,1,bw).astype(np.float32)
        # full-width fade: left side fade in, right side fade out
        fade = np.minimum(ramp, ramp[::-1])*2
        fade = np.clip(fade,0,1)
        fade2 = np.repeat(fade[None,:,None], y1-y0, axis=0)
        blended = (bandimg.astype(np.float32)*(1-fade2) + blurred.astype(np.float32)*fade2).astype(np.uint8)
        img[y0:y1, x0:x1] = blended
    cv2.imwrite(path, img)
    print(f"{path}: blurred seams at {seams}")

if __name__ == "__main__":
    blend_seams(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]))
