import io
f = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\Sexy.TodLib\TodStringFile.cpp"
with io.open(f, "r", encoding="utf-8") as fh:
    c = fh.read()

reps = [
    # 1) types of the per-char state
    ("SexyChar aCurChar = '\\0';\n\tSexyChar aPrevChar = '\\0';",
     "uint32_t aCurChar = 0;\n\tuint32_t aPrevChar = 0;"),
    # 2) decode current code point (advances aDecPos past all its bytes)
    ("aCurChar = theText[aCurPos];",
     "size_t aDecPos = aCurPos;\n\t\taCurChar = Sexy::Utf8Decode(theText, aDecPos);"),
    # 3) space detection only for single-byte chars
    ("else if (CharIsSpaceInFormat(aCurChar, aCurrentFormat))",
     "else if (aCurChar < 256 && CharIsSpaceInFormat((SexyChar)aCurChar, aCurrentFormat))"),
    # 4) newline branch: advance past the newline byte
    ("aSpacePos = aCurPos;\n\t\t\taCurWidth = theRect.mWidth + 1;\n\t\t\taCurPos++;",
     "aSpacePos = aCurPos;\n\t\t\taCurWidth = theRect.mWidth + 1;\n\t\t\taCurPos = (int)aDecPos;"),
    # 5) normal advance: advance past the full code point
    ("aCurPos++;  // 继续下一个字符",
     "aCurPos = (int)aDecPos;  // 继续下一个字符"),
]

for old, new in reps:
    n = c.count(old)
    c = c.replace(old, new)
    print("replaced", n, ":", old[:40])

with io.open(f, "w", encoding="utf-8", newline="\n") as fh:
    fh.write(c)
