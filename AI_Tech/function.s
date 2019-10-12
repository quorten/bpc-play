BITS 32

jmp Action
string db 'If this function executed without errors,',0x0D,0x0A,'then we are a ok good to go!',0x00

Action:
push ebp
mov ebp, esp
push eax
mov eax, [ebp+0x8]
add ebp, 0xC
push ecx
mov ecx, ebp
mov ebp, [ecx]
mov ecx, 0
push edx

CopyLoop:
mov dl, BYTE [eax+string+ecx]
mov BYTE [ebp+ecx], dl
inc ecx
cmp BYTE [eax+string+ecx], 0
jnz CopyLoop
mov BYTE [ebp+ecx], 0

End:
pop edx
pop ecx
pop eax
pop ebp
ret