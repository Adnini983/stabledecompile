import io, re

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
for k in extra:
    print(k)
    print("    " + en[k].encode('unicode_escape').decode('ascii'))
