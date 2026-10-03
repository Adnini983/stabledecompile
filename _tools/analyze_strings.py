import io, os, re

def parse_lawnstrings(path, enc):
    """Parse [KEY] Value [blank] format into dict {key_upper: value}."""
    text = io.open(path,'r',encoding=enc,errors='replace').read()
    d = {}
    # blocks: [KEY]\n value (until next [ or EOF)
    # use regex: \[([^\]]+)\]\r?\n(.*?)(?=\r?\n\[|\Z) with DOTALL
    for m in re.finditer(r'\[([^\]]+)\][ \t]*\r?\n(.*?)(?=\r?\n\[|\Z)', text, re.DOTALL):
        key = m.group(1).strip()
        val = m.group(2)
        # value: trim leading/trailing whitespace, remove newlines
        val = val.strip()
        val = re.sub(r'\r?\n', '', val)
        d[key.upper()] = val
    return d

dep = r'C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\dep_pak\dependency\properties\LawnStrings.txt'
cn  = r'C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\cn_pak\properties\LawnStrings.txt'

en = parse_lawnstrings(dep, 'latin-1')
zh = parse_lawnstrings(cn, 'utf-16')

print("English(dependency) keys:", len(en))
print("Chinese(main.pak) keys:", len(zh))

# Fork-extra keys: in English but not in Chinese
extra = sorted(set(en) - set(zh))
print("\n=== Fork-EXTRA keys (in dependency English, NOT in Chinese main.pak):", len(extra))
for k in extra:
    print("  ", k, "=", en[k][:50].replace(chr(10),' '))

# keys only in Chinese (not needed in supplement but for info)
onlycn = sorted(set(zh) - set(en))
print("\n=== keys ONLY in Chinese (not in dep English):", len(onlycn))

# intersection sample
inter = sorted(set(en) & set(zh))
print("\n=== intersection keys:", len(inter))
# how many intersection values equal (same text)?
diff = [k for k in inter if en[k].strip() != zh[k].strip()]
print("intersection keys where EN value != ZH value:", len(diff), "of", len(inter))
for k in diff[:10]:
    print("   ", k, "| EN=", en[k][:30], "| ZH=", zh[k][:30])
