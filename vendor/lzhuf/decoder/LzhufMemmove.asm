; Manual reconstruction from the pinned retail bytes, not recovered vendor MASM.
; No tested Watcom compiler/source/flags combination emitted this exact object.
; Its register convention and body match the Watcom-built DOS family.
.386
.model flat, C
option casemap:none
option prologue:none
option epilogue:none

EXTERN d_code:BYTE
EXTERN d_len:BYTE
EXTERN dad:WORD
EXTERN decodeSize:DWORD
EXTERN lson:WORD
EXTERN son:WORD
EXTERN prnt:WORD
EXTERN textsize:DWORD
EXTERN text_buf:BYTE
EXTERN rson:WORD
EXTERN codesize:DWORD
EXTERN getbuf:WORD
EXTERN freq:WORD
EXTERN getlen:BYTE
EXTERN codePtr:DWORD
EXTERN match_position:WORD
EXTERN match_length:WORD
EXTERN decodeOutput:DWORD


.code

LzhufMemmove PROC C
    push ecx
    push esi
    push edi
    mov esi,edx
    mov ecx,ebx
    cmp edx,eax
    je L_7fca1
    jae L_7fc8c
    add edx,ebx
    cmp edx,eax
    jbe L_7fc8c
    lea edi,[eax+ebx*1]
    lea esi,[edx-1]
    dec edi
    mov dx,ds
    push es
    ; MASM adds a redundant 66h prefix here; the legacy object did not.
    db 08eh, 0c2h                 ; mov es, dx
    std
    dec esi
    dec edi
    shr ecx,1
    rep movsw
    adc ecx,ecx
    inc esi
    inc edi
    rep movsb
    pop es
    cld
    pop edi
    pop esi
    pop ecx
    ret
L_7fc8c:
    mov dx,ds
    mov edi,eax
    push es
    db 08eh, 0c2h                 ; mov es, dx
    push ecx
    shr ecx,2
    rep movsd
    pop ecx
    and ecx,3
    rep movsb
    pop es
L_7fca1:
    pop edi
    pop esi
    pop ecx
    ret
LzhufMemmove ENDP

END
