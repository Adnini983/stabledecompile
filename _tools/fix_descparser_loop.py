# -*- coding: utf-8 -*-
import io

path = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\SexyAppFramework\DescParser.cpp"
with io.open(path, "r", encoding="utf-8", newline="") as f:
    text = f.read()

p1 = text.find("if (aBuffChar != 0)")
assert p1 != -1, "start anchor not found"
p2 = text.find("if (aChar != '\\r')", p1)
assert p2 != -1, "end anchor not found"

# p0 = position of the 'for (;;)' that owns this body (the last one before p1)
p0 = text.rfind("for (;;)", 0, p1)
assert p0 != -1, "for anchor not found"
# line start of p0 line (keep anything before the for on its line)
ls0 = text.rfind("\n", 0, p0) + 1

# line start of p2 line (keep that line's leading whitespace so 'if (aChar != '\r')' stays indented)
ls2 = text.rfind("\n", 0, p2) + 1

# indentation samples
ls_body = text.rfind("\n", 0, p1) + 1
ind_body = text[ls_body:p1]          # indentation of 'if (aBuffChar != 0)' line
ind_for = ind_body[:-1] if ind_body.endswith("\t") else ind_body

clean = (
    ind_for + "for (;;)\n"
    + ind_for + "{\n"
    + ind_body + "if (aBuffChar != 0)\n"
    + ind_body + "{\n"
    + ind_body + "\taChar = aBuffChar;\n"
    + ind_body + "\taBuffChar = 0;\n"
    + ind_body + "}\n"
    + ind_body + "else if (aPushbackCount > 0)\n"
    + ind_body + "{\n"
    + ind_body + "\taChar = aPushback[0];\n"
    + ind_body + "\tfor (int i = 1; i < aPushbackCount; i++)\n"
    + ind_body + "\t\taPushback[i - 1] = aPushback[i];\n"
    + ind_body + "\taPushbackCount--;\n"
    + ind_body + "}\n"
    + ind_body + "else\n"
    + ind_body + "{\n"
    + ind_body + "\taChar = p_fgetc(aStream);\n"
    + ind_body + "\tif (aChar==EOF)\n"
    + ind_body + "\t\tbreak;\n"
    + ind_body + "}\n"
)

text = text[:ls0] + clean + text[ls2:]

with io.open(path, "w", encoding="utf-8", newline="\n") as f:
    f.write(text)
print("OK - DescParser region replaced from 'for (;;)' to 'if (aChar != '\\r')'")
