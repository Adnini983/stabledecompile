import io
f = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\SexyAppFramework\SysFont.cpp"
with io.open(f, "r", encoding="utf-8") as fh:
    c = fh.read()

anchor = "void SysFont::DrawString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect& theClipRect)\n{\n\tDDImage* aDDImage = dynamic_cast<DDImage*>(g->mDestImage);"
if anchor in c:
    c = c.replace(anchor, anchor + "\n\t// 转成 UTF-16 后交给 GDI 绘制，保证中文等非 ASCII 字符正确显示\n\tstd::wstring aW = Sexy::StringToWString(theString);")
    print("inserted conversion")
else:
    print("ANCHOR NOT FOUND")

n = c.count("theString.c_str(), theString.length()")
c = c.replace("theString.c_str(), theString.length()", "aW.c_str(), (int)aW.length()")
print("replaced TextOut args:", n)

with io.open(f, "w", encoding="utf-8", newline="\n") as fh:
    fh.write(c)
