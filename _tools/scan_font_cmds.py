import io, re
f = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\_tools\cn_pak\data\BrianneTod32.txt"
# supported commands in HandleCommand
handled = {
"Define","CreateHorzSpanRectList","SetDefaultPointSize","SetCharMap","CreateLayer","CreateLayerFrom",
"LayerRequireTags","LayerExcludeTags","LayerPointRange","LayerSetPointSize","LayerSetHeight",
"LayerSetImage","LayerSetDrawMode","LayerSetColorMult","LayerSetColorAdd","LayerSetAscent",
"LayerSetAscentPadding","LayerSetLineSpacingOffset","LayerSetOffset","LayerSetCharWidths",
"LayerSetSpacing","LayerSetImageMap","LayerSetCharOffsets","LayerSetKerningPairs","LayerSetBaseOrder",
"LayerSetCharOrders","LayerSetExInfo"}
with io.open(f, "r", encoding="utf-8-sig", errors="replace") as fh:
    cmds = set()
    for line in fh:
        s = line.rstrip("\r\n")
        if not s.strip():
            continue
        # command lines: first token at column 0 (no leading space)
        if not s[:1].isspace():
            tok = s.split(None, 1)[0] if s.split() else ""
            if tok:
                cmds.add(tok)
missing = sorted(c for c in cmds if c not in handled)
allcmds = sorted(cmds)
print("distinct command tokens:")
for c in allcmds:
    print("  ", c)
print("NOT HANDLED:")
for c in missing:
    print("  !!!", c)
