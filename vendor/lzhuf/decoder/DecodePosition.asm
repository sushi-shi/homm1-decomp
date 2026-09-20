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

EXTERN GetBit:PROC

.code

DecodePosition PROC C
    push ebx
    push ecx
    push edx
    cmp BYTE PTR getlen,8
    jle L_7fd5c
    push ebx
    mov bx,WORD PTR getbuf
    mov eax,ebx
    shl ebx,8
    mov WORD PTR getbuf,bx
    sub BYTE PTR getlen,8
    and eax,65280
    sar eax,8
    pop ebx
    jmp L_7fdb5
L_7fd5c:
    push esi
    mov dl,BYTE PTR getlen
    mov ebx,DWORD PTR codePtr
    mov si,WORD PTR getbuf
L_7fd70:
    mov al,BYTE PTR [ebx]
    xor ah,ah
    movsx ecx,ax
    test ecx,ecx
    jge L_7fd7d
    xor al,al
L_7fd7d:
    mov ecx,8
    sub ecx,edx
    inc ebx
    shl eax,cl
    add dl,8
    or esi,eax
    cmp dl,8
    jle L_7fd70
    mov eax,esi
    shl esi,8
    mov WORD PTR getbuf,si
    and eax,65280
    sub dl,8
    sar eax,8
    mov BYTE PTR getlen,dl
    mov DWORD PTR codePtr,ebx
    pop esi
L_7fdb5:
    mov edx,eax
    and edx,65535
    xor bh,bh
    mov bl,BYTE PTR [edx+d_code]
    mov ecx,ebx
    mov dl,BYTE PTR [edx+d_len]
    shl ecx,6
    xor dh,dh
    sub edx,2
L_7fdd5:
    dec edx
    cmp dx,65535
    je L_7fde9
    mov ebx,eax
    add ebx,eax
    call GetBit
    add eax,ebx
    jmp L_7fdd5
L_7fde9:
    xor ah,ah
    and al,63
    or eax,ecx
    pop edx
    pop ecx
    pop ebx
    ret
DecodePosition ENDP

END
