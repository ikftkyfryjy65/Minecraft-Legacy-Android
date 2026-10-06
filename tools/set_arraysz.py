import struct, sys
path = sys.argv[1]
newval = int(sys.argv[2])
apply = len(sys.argv) > 3 and sys.argv[3] == "apply"
d = bytearray(open(path, "rb").read())
assert d[:4] == b"\x7fELF" and d[4] == 2 and d[5] == 1, "not ELF64 LE"
phoff = struct.unpack_from("<Q", d, 0x20)[0]
phentsize, phnum = struct.unpack_from("<HH", d, 0x36)
found = False
for i in range(phnum):
    base = phoff + i * phentsize
    ptype = struct.unpack_from("<I", d, base)[0]
    if ptype != 2:
        continue
    off, = struct.unpack_from("<Q", d, base + 8)
    size, = struct.unpack_from("<Q", d, base + 32)
    p = off
    while p < off + size:
        tag, val = struct.unpack_from("<qQ", d, p)
        if tag == 0:
            break
        if tag == 0x1b:
            print("found at file offset", hex(p + 8), "old value", val)
            found = True
            if apply:
                struct.pack_into("<Q", d, p + 8, newval)
        p += 16
if not found:
    sys.exit("INIT_ARRAYSZ not found")
if apply:
    open(path, "wb").write(d)
    print("WRITTEN, new value", newval)
else:
    print("dry-run only, nothing written")
