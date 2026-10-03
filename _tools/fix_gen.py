import re, io

f = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\SexyAppFramework\ImageFont.cpp"
with io.open(f, "r", encoding="utf-8") as fh:
    c = fh.read()

orig = c

# Loop 1 (scaled=1.0): copy mImageRect for every defined glyph
c, n1 = re.subn(
    r"for \(int aCharNum = 0; aCharNum < 256; aCharNum\+\+\)\n(\s*)anActiveFontLayer->mScaledCharImageRects\[aCharNum\] = aFontLayer->mCharData\[aCharNum\]\.mImageRect;",
    "for (std::map<uint32_t, CharData>::const_iterator aItr = aFontLayer->mCharData.begin();\n"
    "\\1\taItr != aFontLayer->mCharData.end(); ++aItr)\n"
    "\\1\tanActiveFontLayer->mScaledCharImageRects[aItr->first] = aItr->second.mImageRect;",
    c)

# Loop 2 (scaled!=1.0): build scaled rects over every defined glyph
c, n2 = re.subn(
    r"// Resize font elements\n\s*int aCharNum;\n\s*SDL3Image\* aMemoryImage = new SDL3Image\(LawnApp::mSDLRenderer\);",
    "// Resize font elements\n"
    "SDL3Image* aMemoryImage = new SDL3Image(LawnApp::mSDLRenderer);",
    c)
c, n2b = re.subn(
    r"for \(aCharNum = 0; aCharNum < 256; aCharNum\+\+\)\n(\s*)\{\n(\s*)Rect\* anOrigRect = &aFontLayer->mCharData\[aCharNum\]\.mImageRect;",
    "for (std::map<uint32_t, CharData>::const_iterator aItr = aFontLayer->mCharData.begin();\n"
    "\\1\taItr != aFontLayer->mCharData.end(); ++aItr)\n"
    "\\1{\n"
    "\\2uint32_t aCharNum = aItr->first;\n"
    "\\2Rect* anOrigRect = &aFontLayer->mCharData[aCharNum].mImageRect;",
    c)

# Loop 3 (draw scaled atlas over every defined glyph)
c, n3 = re.subn(
    r"for \(aCharNum = 0; aCharNum < 256; aCharNum\+\+\)\n(\s*)\{\n(\s*)if \(\(Image\*\) aFontLayer->mImage != NULL\)",
    "for (std::map<uint32_t, CharData>::const_iterator aItr = aFontLayer->mCharData.begin();\n"
    "\\1\taItr != aFontLayer->mCharData.end(); ++aItr)\n"
    "\\1{\n"
    "\\2uint32_t aCharNum = aItr->first;\n"
    "\\2if ((Image*) aFontLayer->mImage != NULL)",
    c)

with io.open(f, "w", encoding="utf-8", newline="\n") as fh:
    fh.write(c)

print("loop1:", n1, "loop2decl:", n2, "loop2:", n2b, "loop3:", n3)
