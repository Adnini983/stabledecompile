import re, io, sys

f = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\SexyAppFramework\ImageFont.cpp"
with io.open(f, "r", encoding="utf-8") as fh:
    c = fh.read()

orig = c

# 1) CharWidths / CharOrders:  "if (aCharsVector[i].length() == 1)" (no &&) -> decode single UTF-8 char
c = c.replace(
    "\t\t\t\t\t\t\t\tif (aCharsVector[i].length() == 1)",
    "\t\t\t\t\t\t\t\tuint32_t aCp;\n\t\t\t\t\t\t\t\tif (TodUtf8SingleChar(aCharsVector[i], aCp))")

# 2) ImageMap / CharOffsets:  "if ((aCharsVector[i].length() == 1) &&" -> decode single UTF-8 char
c = c.replace(
    "\t\t\t\t\t\t\t\tif ((aCharsVector[i].length() == 1) &&",
    "\t\t\t\t\t\t\t\tuint32_t aCp;\n\t\t\t\t\t\t\t\tif (TodUtf8SingleChar(aCharsVector[i], aCp) &&")

# 3) CharWidths assignment
c = c.replace(
    "aLayer->mCharData[(uchar) aCharsVector[i][0]].mWidth = ",
    "aLayer->mCharData[aCp].mWidth = ")
# 4) CharOrders assignment
c = c.replace(
    "aLayer->mCharData[(uchar) aCharsVector[i][0]].mOrder = ",
    "aLayer->mCharData[aCp].mOrder = ")
# 5) ImageMap assignment (note trailing tabs after ';;')
c = re.sub(r"aLayer->mCharData\[\(uchar\) aCharsVector\[i\]\[0\]\]\.mImageRect = aRect;;\s*$",
           "aLayer->mCharData[aCp].mImageRect = aRect;", c, flags=re.M)
# 6) CharOffsets assignment
c = c.replace(
    "aLayer->mCharData[(uchar) aCharsVector[i][0]].mOffset = ",
    "aLayer->mCharData[aCp].mOffset = ")

# 7) ImageMap defaultHeight loop: iterate the glyph map instead of 256
c = re.sub(
    r"aLayer->mDefaultHeight = 0;\s*\n\s*for \(int aCharNum = 0; aCharNum < 256; aCharNum\+\+\)\s*\n\s*if \(aLayer->mCharData\[aCharNum\]\.mImageRect\.mHeight \+ aLayer->mCharData\[aCharNum\]\.mOffset\.mY > aLayer->mDefaultHeight\)\s*\n\s*aLayer->mDefaultHeight = aLayer->mCharData\[aCharNum\]\.mImageRect\.mHeight \+ aLayer->mCharData\[aCharNum\]\.mOffset\.mY;",
    "\t\t\t\t\t\t\taLayer->mDefaultHeight = 0;\n"
    "\t\t\t\t\t\t\tfor (std::map<uint32_t, CharData>::const_iterator aItr = aLayer->mCharData.begin();\n"
    "\t\t\t\t\t\t\t\taItr != aLayer->mCharData.end(); ++aItr)\n"
    "\t\t\t\t\t\t\t\tif (aItr->second.mImageRect.mHeight + aItr->second.mOffset.mY > aLayer->mDefaultHeight)\n"
    "\t\t\t\t\t\t\t\t\taLayer->mDefaultHeight = aItr->second.mImageRect.mHeight + aItr->second.mOffset.mY;",
    c)

# 8) KerningPairs: keep for ASCII (<256) pairs
c = re.sub(
    r"if \(aPairsVector\[i\]\.length\(\) == 2\)\s*\n(\s*)aLayer->mCharData\[\(uchar\) aPairsVector\[i\]\[0\]\]\.mKerningOffsets\s*\n\s*\[\(uchar\) aPairsVector\[i\]\[1\]\] = anOffsetsVector\[i\];",
    "if (aPairsVector[i].length() == 2)\n"
    "\\1uint32_t aK0 = (uint32_t)(unsigned char)aPairsVector[i][0];\n"
    "\\1uint32_t aK1 = (uint32_t)(unsigned char)aPairsVector[i][1];\n"
    "\\1if (aK0 < 256 && aK1 < 256)\n"
    "\\1\\taLayer->mCharData[aK0].mKerningOffsets[aK1] = anOffsetsVector[i];",
    c)

with io.open(f, "w", encoding="utf-8", newline="\n") as fh:
    fh.write(c)

print("changed bytes:", len(orig) != len(c))
# report any remaining old patterns
for pat in [r"mCharData\[\(uchar\)", r"mCharMap\[\(uchar\)", r"mScaledCharImageRects\[\(uchar\)", r"mCharData\[aCharNum\]", r"mScaledCharImageRects\[aCharNum\]"]:
    m = re.findall(pat, c)
    print(pat, "remaining:", len(m))
