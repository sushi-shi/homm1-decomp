; HoMM1 resource-name hash used by resourceManager::MakeId.

.386
.model flat
option casemap:none
option prologue:none
option epilogue:none

.code

; C++ equivalent (include/BASE/MAKEFILEID.h: unsigned long MAKEFILEID(char*),
; __cdecl). The hash runs in the 16-bit AX; EAX starts at zero, so the result
; always has a zero high word. EBX is used as scratch without being saved.
; HoMM2's MAKEFILEID (Buka BASE/Misc.cpp) is a different, C++ hash.
;
;   unsigned long MAKEFILEID(char* name) {
;       unsigned short hash = 0;
;       for (; *name; ++name) {
;           unsigned char c = *name & 0x7f;
;           if (c >= 0x60)                       // fold 'a'..'z' (and `{|}~DEL)
;               c -= 0x20;
;           hash = (unsigned short)((hash >> 8) | (hash << 8));   // xchg al, ah
;           hash = (unsigned short)((hash << 1) | (hash >> 15));  // rol ax, 1
;           hash -= c;
;       }
;       return hash;
;   }
?MAKEFILEID@@YAIPAD@Z PROC
    push ebp
    mov ebp, esp
    push esi
    mov esi, DWORD PTR [ebp+8]
    sub ebx, ebx
    sub eax, eax
hash_next:
    mov bl, BYTE PTR [esi]
    or bl, bl
    jz hash_done
    and bl, 07fh
    cmp bl, 060h
    jb hash_folded
    sub bl, 020h
hash_folded:
    xchg al, ah
    rol ax, 1
    sub ax, bx
    inc esi
    jmp hash_next
hash_done:
    pop esi
    pop ebp
    ret
?MAKEFILEID@@YAIPAD@Z ENDP

END
