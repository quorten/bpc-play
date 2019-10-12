/*

This works.

0C00- 68 2E 0A 00 00 68 48 65 X2A23
0C08- 63 6B 89 E1 68 06 00 00 X828C
0C10- 00 51 68 00 00 00 00 E8 X68B9
0C18- 6C FC FF FF 58 58 58 58 X9303
0C20- 58 C3 00 00 00 00 00 00 X58C3
0C00G

The idea is to code in common with the monitor ROM, using
some of the monitor routines as if they are "I/O firmware"
to abstract platform-specific differences.

*/

pushl $0xa2e
pushl $0x6b636548
mov %esp, %ecx
pushl $0x00000006
push %ecx
pushl $0x00000000
# 0888 - 0c15 = fc73
# Add 4 bytes because 4 will be automatically subtracted.
call 0xfffffc8c
pop %eax
pop %eax
pop %eax
pop %eax
pop %eax
ret
