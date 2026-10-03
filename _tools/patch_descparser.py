# -*- coding: utf-8 -*-
import io, sys

path = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\SexyAppFramework\DescParser.cpp"
with io.open(path, "r", encoding="utf-8", newline="") as f:
    text = f.read()

# --- 1) Insert BOM-strip block right after 'char aBuffChar = 0;' ---
anchor = "char aBuffChar = 0;"
idx = text.find(anchor)
assert idx != -1, "anchor aBuffChar not found"
line_start = text.rfind("\n", 0, idx) + 1
indent = text[line_start:idx]  # whitespace before 'char aBuffChar'
bom_block = (
    indent + "char aBuffChar = 0;\n"
    "\n"
    + indent + "// 豆包修复：剥离 UTF-8 BOM（EF BB BF）。\n"
    + indent + "// 中文年度版字体描述文件（如 BrianneTod*.txt）为 UTF-8 带 BOM 编码，\n"
    + indent + "// 若不去除 BOM，EF BB BF 会被当作首字符参与解析导致失败。\n"
    + indent + "int aPushback[3] = { 0, 0, 0 };\n"
    + indent + "int aPushbackCount = 0;\n"
    + indent + "{\n"
    + indent + "\tint aB0 = p_fgetc(aStream);\n"
    + indent + "\tint aB1 = p_fgetc(aStream);\n"
    + indent + "\tint aB2 = p_fgetc(aStream);\n"
    + indent + "\tif (aB0 == 0xEF && aB1 == 0xBB && aB2 == 0xBF)\n"
    + indent + "\t{\n"
    + indent + "\t\t// 命中 UTF-8 BOM：直接丢弃，从 BOM 之后继续解析\n"
    + indent + "\t}\n"
    + indent + "\telse\n"
    + indent + "\t{\n"
    + indent + "\t\t// 不是 BOM：按原顺序回放，保证与未改动前行为一致\n"
    + indent + "\t\tif (aB0 != EOF) aPushback[aPushbackCount++] = aB0;\n"
    + indent + "\t\tif (aB1 != EOF) aPushback[aPushbackCount++] = aB1;\n"
    + indent + "\t\tif (aB2 != EOF) aPushback[aPushbackCount++] = aB2;\n"
    + indent + "\t}\n"
    + indent + "}\n"
)
text = text[:idx] + bom_block + text[idx + len(anchor):]

# --- 2) Modify the char-source chain: consult aPushback before p_fgetc ---
old_else = "else\n\t\t\t\t{\n\t\t\t\t\taChar = p_fgetc(aStream);\n\t\t\t\t\tif (aChar==EOF)\n\t\t\t\t\t\tbreak;\n\t\t\t\t}"
# fallback: locate by 'aChar = p_fgetc(aStream);'
if old_else not in text:
    # derive indentation from the line containing 'aChar = p_fgetc(aStream);'
    j = text.find("aChar = p_fgetc(aStream);")
    assert j != -1, "p_fgetc line not found"
    ls = text.rfind("\n", 0, j) + 1
    ind = text[ls:j]
    new_else = (
        "else if (aPushbackCount > 0)\n"
        + ind + "{\n"
        + ind + "\taChar = aPushback[0];\n"
        + ind + "\tfor (int i = 1; i < aPushbackCount; i++)\n"
        + ind + "\t\taPushback[i - 1] = aPushback[i];\n"
        + ind + "\taPushbackCount--;\n"
        + ind + "}\n"
        + ind + "else\n"
        + ind + "{\n"
        + ind + "\taChar = p_fgetc(aStream);\n"
        + ind + "\tif (aChar==EOF)\n"
        + ind + "\t\tbreak;\n"
        + ind + "}"
    )
    # replace only the exact old else branch: from the 'else' that precedes the p_fgetc line
    old_block_start = text.rfind("else", 0, j)
    # ensure it's the standalone 'else' (followed by newline + brace)
    seg = text[old_block_start:j + len("aChar = p_fgetc(aStream);")]
    text = text[:old_block_start] + new_else + text[j + len("aChar = p_fgetc(aStream);"):]
else:
    text = text.replace(old_else, new_else, 1)

with io.open(path, "w", encoding="utf-8", newline="\n") as f:
    f.write(text)
print("OK - DescParser.cpp patched")
