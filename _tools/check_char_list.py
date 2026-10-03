import io, re
f = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\cn_pak\data\BrianneTod32.txt"
with io.open(f,"r",encoding="utf-8-sig",errors="replace") as fh:
    text=fh.read()

def cps(t):
    n=0; i=0
    while i < len(t):
        b=ord(t[i]); n+=1
        if b<0x80: i+=1
        elif b<0xE0: i+=2
        elif b<0xF0: i+=3
        else: i+=4
    return n

def unescape(t):
    out=""; i=0
    while i < len(t):
        if t[i]=='\\' and i+1<len(t):
            out+=t[i+1]; i+=2
        else:
            out+=t[i]; i+=1
    return out

# find Define CharList0 start
start=text.find("Define CharList0")
# find the terminating ';' (after the opening paren on the following lines)
idx=text.find("(", start)
# scan forward for the ';' that closes (respecting quotes)
i=idx; sq=dq=False; semi=-1
while i < len(text):
    c=text[i]
    if c=="'" and not dq: sq=not sq
    elif c=='"' and not sq: dq=not dq
    elif c==';' and not sq and not dq: semi=i; break
    i+=1
block=text[start:semi+1]
print("block len:", len(block))
toks=re.findall(r"'((?:[^'\\]|\\.)*)'", block)
print("quoted tokens:", len(toks))
bad=[]
for idx2,t in enumerate(toks):
    u=unescape(t)
    if cps(u)!=1:
        bad.append((idx2, t, u, cps(u)))
print("NON single-codepoint entries:", len(bad))
for b in bad[:20]:
    print("   idx", b[0], "rawrepr=", repr(b[1]), "unescaped=", repr(b[2]), "cps=", b[3])
