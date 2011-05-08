#!/usr/bin/env python
"""
This script reads all symbols in kernel.bin and writes a NASM file called
symbols.asm with an array of the symbols and their addresses.
Used by kernel_debugpanic() ;)

"""
from string import Template
import sys

sym_template = Template("""section .data:

global __symbols
global __symbols_total

__symbols:
$entries

__symbols_total: dd $total

$strings""")

if '--null' in sys.argv:
    total = 0
    entries = '  dq 0'
    strings = ''
else:
    symbols = {}
    for line in sys.stdin:
        addr, string = line.split()
        symbols[addr] = string

    total = len(symbols)
    entries = '\n'.join("  dd {0}h\n  dd s{0}".format(addr)
                    for addr in sorted(symbols.keys()))

    strings = '\n'.join("s{0} db '{1}', 0".format(addr, symbols[addr])
                    for addr in sorted(symbols.keys()))

print(sym_template.substitute(
    entries=entries,
    total=total,
    strings=strings
))

