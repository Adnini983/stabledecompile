import io
f = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\SexyAppFramework\SysFont.cpp"
with io.open(f, "r", encoding="utf-8") as fh:
    c = fh.read()

# 1) DrawTextExW needs LPWSTR (mutable) -> cast
c = c.replace("DrawTextExW(aDC, aW.c_str(), (int)aW.length(), &aRect, DT_CALCRECT | DT_NOPREFIX, NULL);",
              "DrawTextExW(aDC, (LPWSTR)aW.c_str(), (int)aW.length(), &aRect, DT_CALCRECT | DT_NOPREFIX, NULL);")
# 2) In narrow builds TextOut->TextOutA; we render wide string, use TextOutW explicitly
c = c.replace("TextOut(", "TextOutW(")

with io.open(f, "w", encoding="utf-8", newline="\n") as fh:
    fh.write(c)
print("done")
