import os, hashlib

dep_img=r'C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\dep_pak_full\dependency\images'
cn_img =r'C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\cn_pak\images'

both=['CobCannon_target_shadow.png','Night_grave_graphic.png','PopCap_Logo.jpg','Unlock_Highlight.png','Unlock_Normal.png','Unlock_Press.png','background5_gameover_mask.png','background6_gameover_mask.png','quickplay_back_button.png','quickplay_back_button_highlight.png','quickplay_minigames_button.png','quickplay_minigames_button_highlight.png','quickplay_minigames_cloud.png','quickplay_puzzles_button.png','quickplay_puzzles_button_highlight.png','quickplay_puzzles_cloud.png','quickplay_survival_button.png','quickplay_survival_button_highlight.png','quickplay_survival_cloud.png','selector_morewaystoplay_background.png','selectorscreen_achievements_bg.png','zombatar_back_button.png','zombatar_back_button_highlight.png','zombatar_main_bg.png','zombatar_mainmenuback_highlight.png']

def sha(p):
    with open(p,'rb') as f: return hashlib.md5(f.read()).hexdigest()

print("=== identical (no change needed) ===")
same=[]; diff=[]
for rel in both:
    dp=os.path.join(dep_img,rel); cp=os.path.join(cn_img,rel)
    sd=sha(dp); sc=sha(cp)
    tag = "SAME" if sd==sc else "DIFF"
    dsz=os.path.getsize(dp); csz=os.path.getsize(cp)
    if sd==sc: same.append(rel)
    else: diff.append((rel,dsz,csz))
    print(f"{tag:4} dep={dsz:7} cn={csz:7}  {rel}")
print("\n=== DIFF count:",len(diff)," SAME count:",len(same))
