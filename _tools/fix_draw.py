import io
f = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\SexyAppFramework\ImageFont.cpp"
with io.open(f, "r", encoding="utf-8") as fh:
    c = fh.read()

# kerning: codepoint-aware accessor (do this BEFORE the plain mCharData replace)
k = c.count("mCharData[(uchar) aChar].mKerningOffsets[(uchar) aNextChar]")
c = c.replace("mCharData[(uchar) aChar].mKerningOffsets[(uchar) aNextChar]",
              "mCharData[aChar].GetKerningOffset(aNextChar)")
n1 = c.count("mCharData[(uchar) aChar]")
c = c.replace("mCharData[(uchar) aChar]", "mCharData[aChar]")
n2 = c.count("mScaledCharImageRects[(uchar) aChar]")
c = c.replace("mScaledCharImageRects[(uchar) aChar]", "mScaledCharImageRects[aChar]")

with io.open(f, "w", encoding="utf-8", newline="\n") as fh:
    fh.write(c)

print("kerning:", k, "mCharData(uchar):", n1, "mScaled(uchar):", n2)
