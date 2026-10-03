import io
f = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\Sexy.TodLib\TodCommon.cpp"
with io.open(f, "r", encoding="utf-8") as fh:
    c = fh.read()

reps = [
    (
     "for (int aCharNum = 0; aCharNum < (int)aFinalString.size(); aCharNum++)\n\t{\n\t\tSexyChar aChar = aFont->GetMappedChar(aFinalString[aCharNum]);\n\t\tSexyChar aNextChar = '\\0';\n\t\tif (aCharNum < (int)aFinalString.size() - 1)\n\t\t{\n\t\t\taNextChar = aFont->GetMappedChar(aFinalString[aCharNum + 1]);\n\t\t}",
     "std::vector<uint32_t> aCps;\n\tSexy::Utf8ToCodePoints(aFinalString, aCps);\n\tfor (int aCharNum = 0; aCharNum < (int)aCps.size(); aCharNum++)\n\t{\n\t\tuint32_t aChar = aFont->GetMappedChar(aCps[aCharNum]);\n\t\tuint32_t aNextChar = 0;\n\t\tif (aCharNum < (int)aCps.size() - 1)\n\t\t{\n\t\t\taNextChar = aFont->GetMappedChar(aCps[aCharNum + 1]);\n\t\t}"
    ),
    ("aSpacing += aCharData->mKerningOffsets[aNextChar];",
     "aSpacing += aCharData->GetKerningOffset(aNextChar);"),
    ("aSpacing += aCharData->mKerningOffsets[aNextChar] * aScale;",
     "aSpacing += aCharData->GetKerningOffset(aNextChar) * aScale;"),
]
for old, new in reps:
    n = c.count(old)
    c = c.replace(old, new)
    print("replaced", n, ":", old[:50])
with io.open(f, "w", encoding="utf-8", newline="\n") as fh:
    fh.write(c)
