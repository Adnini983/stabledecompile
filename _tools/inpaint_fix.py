#!/usr/bin/env python3
"""豆包修复：用 OpenCV 内容感知修复(inpaint)自然填充场地背景的裸地/阴影区域。
用法: python inpaint_fix.py <input> <output> <mask_rect_csv>
mask_rect_csv: 逗号分隔的 x0,y0,x1,y1 矩形（单个），或分号分隔多个矩形。
用原背景的草坪纹理作源，Telea 方法传播填充，无平铺复制痕迹。
"""
import sys, numpy as np, cv2

def inpaint_fix(src, dst, rects):
    img = cv2.imread(src)          # BGR
    if img is None:
        raise SystemExit(f"cannot read {src}")
    h, w = img.shape[:2]
    mask = np.zeros((h, w), np.uint8)
    for (x0, y0, x1, y1) in rects:
        x0 = max(0, min(w, x0)); x1 = max(0, min(w, x1))
        y0 = max(0, min(h, y0)); y1 = max(0, min(h, y1))
        mask[y0:y1, x0:x1] = 255
    # dilate mask slightly so inpaint blends across the seam
    mask = cv2.dilate(mask, np.ones((3,3), np.uint8), iterations=1)
    out = cv2.inpaint(img, mask, 3, cv2.INPAINT_TELEA)
    cv2.imwrite(dst, out)
    print(f"{src} -> {dst}: inpainted rects={rects}, size {w}x{h}")

if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    rectstr = sys.argv[3]
    rects = []
    for grp in rectstr.split(';'):
        parts = [int(v) for v in grp.split(',')]
        if len(parts) == 4:
            rects.append(tuple(parts))
    inpaint_fix(src, dst, rects)
