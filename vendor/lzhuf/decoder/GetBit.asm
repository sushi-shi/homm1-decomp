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

GetBit PROC C
    cmp BYTE PTR getlen,8
    jle L_7fcd1
    push ebx
    mov bx,WORD PTR getbuf
    mov eax,ebx
    add ebx,ebx
    mov WORD PTR getbuf,bx
    and eax,32768
    sar eax,15
    dec BYTE PTR getlen
    pop ebx
    ret
L_7fcd1:
    push ebx
    push ecx
    push edx
    push esi
    mov ebx,DWORD PTR codePtr
    mov dl,BYTE PTR getlen
    mov si,WORD PTR getbuf
L_7fce8:
    xor eax,eax
    mov al,BYTE PTR [ebx]
    test eax,eax
    jge L_7fcf0
L_7fcf0:
    mov ecx,8
    sub ecx,edx
    inc ebx
    shl eax,cl
    add dl,8
    or esi,eax
    cmp dl,8
    jle L_7fce8
    mov eax,esi
    add esi,esi
    mov WORD PTR getbuf,si
    and eax,32768
    dec dl
    sar eax,15
    mov BYTE PTR getlen,dl
    mov DWORD PTR codePtr,ebx
    pop esi
    pop edx
    pop ecx
    pop ebx
    ret
GetBit ENDP

END
