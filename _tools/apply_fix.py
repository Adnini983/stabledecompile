import io, os, re, shutil

def parse_lawnstrings(path, enc):
    text = io.open(path,'r',encoding=enc,errors='replace').read()
    d = {}
    for m in re.finditer(r'\[([^\]]+)\][ \t]*\r?\n(.*?)(?=\r?\n\[|\Z)', text, re.DOTALL):
        key = m.group(1).strip()
        val = re.sub(r'\r?\n', '', m.group(2).strip())
        d[key.upper()] = val
    return d

dep = r'C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\dep_pak\dependency\properties\LawnStrings.txt'
cn  = r'C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\cn_pak\properties\LawnStrings.txt'
en = parse_lawnstrings(dep, 'latin-1')
zh = parse_lawnstrings(cn, 'utf-16')

extra = sorted(set(en) - set(zh))
print("Fork-extra keys to supplement:", len(extra))

# Write supplement LawnStrings.txt
out = r'C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\dep_pak_full\dependency\properties\LawnStrings.txt'
lines = []
for k in extra:
    v = en[k]
    lines.append(f"[{k}]\n{v}\n\n")
with io.open(out,'w',encoding='latin-1',newline='') as f:
    f.write(''.join(lines))
print("wrote supplement:", out, os.path.getsize(out), "bytes,", len(extra), "keys")

# Replace 21 differing shared images with main.pak Chinese versions
dep_img=r'C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\dep_pak_full\dependency\images'
cn_img =r'C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\cn_pak\images'
both=['Night_grave_graphic.png','PopCap_Logo.jpg','Unlock_Highlight.png','Unlock_Normal.png','Unlock_Press.png','background5_gameover_mask.png','background6_gameover_mask.png','quickplay_back_button.png','quickplay_back_button_highlight.png','quickplay_minigames_button.png','quickplay_minigames_button_highlight.png','quickplay_puzzles_button.png','quickplay_puzzles_button_highlight.png','quickplay_survival_button.png','quickplay_survival_button_highlight.png','selector_morewaystoplay_background.png','selectorscreen_achievements_bg.png','zombatar_back_button.png','zombatar_back_button_highlight.png','zombatar_main_bg.png','zombatar_mainmenuback_highlight.png']
for rel in both:
    shutil.copyfile(os.path.join(cn_img,rel), os.path.join(dep_img,rel))
print("replaced", len(both), "images with main.pak Chinese versions")
