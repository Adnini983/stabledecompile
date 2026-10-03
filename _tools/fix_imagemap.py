import io
f = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\SexyAppFramework\ImageFont.cpp"
with io.open(f, "r", encoding="utf-8") as fh:
    lines = fh.read().split("\n")

changed = 0
i = 0
while i < len(lines):
    line = lines[i]
    # --- ImageMap: (length()==1) && ... size()==4 ---
    if ("(aCharsVector[i].length() == 1) &&" in line and i+2 < len(lines)
            and "(aRectElement.size() == 4))" in lines[i+2]):
        # find preceding 'IntVector aRectElement;'
        j = i
        while j > 0 and "IntVector aRectElement;" not in lines[j]:
            j -= 1
        indent = lines[i][:len(lines[i]) - len(lines[i].lstrip())]
        lines[j] = lines[j] + "\n" + indent + "uint32_t aCp;"
        lines[i] = lines[i].replace("(aCharsVector[i].length() == 1) &&",
                                    "(TodUtf8SingleChar(aCharsVector[i], aCp)) &&")
        changed += 1
    # --- CharOffsets: (length()==1) && ... size()==2 ---
    elif ("(aCharsVector[i].length() == 1) &&" in line and i+2 < len(lines)
            and "(aRectElement.size() == 2))" in lines[i+2]):
        j = i
        while j > 0 and "IntVector aRectElement;" not in lines[j]:
            j -= 1
        indent = lines[i][:len(lines[i]) - len(lines[i].lstrip())]
        lines[j] = lines[j] + "\n" + indent + "uint32_t aCp;"
        lines[i] = lines[i].replace("(aCharsVector[i].length() == 1) &&",
                                    "(TodUtf8SingleChar(aCharsVector[i], aCp)) &&")
        changed += 1
    # --- CharOrders: if (aCharsVector[i].length() == 1) ... mCharData[aCp].mOrder ---
    elif ("if (aCharsVector[i].length() == 1)" in line):
        # confirm the following block uses aCp
        look = "\n".join(lines[i:i+5])
        if "mCharData[aCp].mOrder" in look:
            indent = lines[i][:len(lines[i]) - len(lines[i].lstrip())]
            lines.insert(i, indent + "uint32_t aCp;")
            # now the original 'if' line shifted to i+1
            lines[i+1] = lines[i+1].replace("if (aCharsVector[i].length() == 1)",
                                            "if (TodUtf8SingleChar(aCharsVector[i], aCp))")
            changed += 1
            i += 1  # skip inserted line
    i += 1

with io.open(f, "w", encoding="utf-8", newline="\n") as fh:
    fh.write("\n".join(lines))
print("changed:", changed)
