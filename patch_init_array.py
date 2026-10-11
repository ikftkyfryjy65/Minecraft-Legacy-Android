"""Zero out all .init_array entries except the first ~30 (which handle libc/our infrastructure)."""
import struct, sys

BIN = 'Linux/Debug/Minecraft.Client'

# Read ELF section header offset
with open(BIN, 'rb') as f:
    data = bytearray(f.read())

# ELF64 header
e_shoff = struct.unpack('<Q', data[0x28:0x30])[0]
e_shentsize = struct.unpack('<H', data[0x3a:0x3c])[0]
e_shnum = struct.unpack('<H', data[0x3c:0x3e])[0]
e_shstrndx = struct.unpack('<H', data[0x3e:0x40])[0]

# Find section name table
sh_strtab_off = struct.unpack('<Q', data[e_shoff + e_shstrndx * e_shentsize + 0x18:e_shoff + e_shstrndx * e_shentsize + 0x20])[0]

def get_section_name(idx):
    off = e_shoff + idx * e_shentsize
    name_idx = struct.unpack('<I', data[off:off+4])[0]
    end = data.index(b'\0', sh_strtab_off + name_idx)
    return data[sh_strtab_off + name_idx:end].decode()

# Find .init_array
init_array_found = None
for i in range(e_shnum):
    name = get_section_name(i)
    if name == '.init_array':
        off = e_shoff + i * e_shentsize
        sh_offset = struct.unpack('<Q', data[off + 0x18:off + 0x20])[0]
        sh_size = struct.unpack('<Q', data[off + 0x20:off + 0x28])[0]
        init_array_found = (sh_offset, sh_size)
        print(f".init_array at file offset 0x{sh_offset:x}, size {sh_size} bytes ({sh_size//8} entries)")
        break

if not init_array_found:
    print("Not found"); sys.exit(1)

off, size = init_array_found
entries = size // 8

# Zero out entries AFTER the first N (keep first N for infra)
KEEP_FIRST = 30  # our constructors use priorities 101,102,103,500 etc → these are the FIRST entries after libc's

for i in range(KEEP_FIRST, entries):
    pos = off + i * 8
    data[pos:pos+8] = b'\x00' * 8

with open(BIN, 'wb') as f:
    f.write(data)

print(f"Zeroed entries {KEEP_FIRST}..{entries-1} (kept first {KEEP_FIRST})")
