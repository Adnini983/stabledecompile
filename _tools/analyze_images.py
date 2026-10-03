import os

def list_rel(root):
    out=[]
    for dp,dn,fn in os.walk(root):
        for f in fn:
            full=os.path.join(dp,f)
            rel=os.path.relpath(full,root)
            out.append(rel)
    return out

dep_img=r'C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\dep_pak_full\dependency\images'
cn_img =r'C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\cn_pak\images'

dep=list_rel(dep_img)
cn=list_rel(cn_img)

print("dependency images:",len(dep))
print("cn(main.pak) images:",len(cn))

depset=set(dep); cnset=set(cn)
both=sorted(depset & cnset)
print("\n=== images in BOTH (dependency overrides main.pak):", len(both))
for p in both:
    print("  ",p)

print("\n=== images ONLY in dependency (Fork-added, no cn version):", len(depset-cnset))
for p in sorted(depset-cnset)[:30]:
    print("  ",p)
