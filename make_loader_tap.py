#!/usr/bin/env python3
import struct

def make_tap_block(flag, data):
    block = bytes([flag]) + data
    checksum = 0
    for b in block:
        checksum ^= b
    block += bytes([checksum])
    return struct.pack('<H', len(block)) + block

def make_basic_line(line_num, tokens):
    data = tokens + b'\x0d'
    return struct.pack('>H', line_num) + struct.pack('<H', len(data)) + data

def num_token(n):
    s = str(n).encode('ascii')
    return s + b'\x0e\x00\x00' + struct.pack('<H', n) + b'\x00'

TK_CLEAR = b'\xfd'
TK_LOAD  = b'\xef'
TK_CODE  = b'\xaf'
TK_RAND  = b'\xf9'
TK_USR   = b'\xc0'
TK_QUOTE = b'"'

line10 = make_basic_line(10, TK_CLEAR + num_token(24575))
line20 = make_basic_line(20, TK_LOAD + TK_QUOTE + TK_QUOTE + TK_CODE)
line30 = make_basic_line(30, TK_LOAD + TK_QUOTE + TK_QUOTE + TK_CODE)
line40 = make_basic_line(40, TK_RAND + TK_USR + num_token(32768))

basic_data = line10 + line20 + line30 + line40

filename = b'loader    '
header_data = bytes([0x00]) + filename + struct.pack('<H', len(basic_data)) + struct.pack('<H', 10) + struct.pack('<H', len(basic_data))

header_block = make_tap_block(0x00, header_data)
data_block = make_tap_block(0xFF, basic_data)

with open('loader.tap', 'wb') as f:
    f.write(header_block)
    f.write(data_block)

print(f"Created loader.tap ({len(basic_data)} bytes of BASIC)")
