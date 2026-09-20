; Legacy LZHUF decoder object linked into the VC4 Windows build.  Its register
; convention and bodies match the Watcom-built DOS family; that does not imply
; the Windows build itself invoked Watcom.
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

EXTERN LzhufMemmove:PROC

.code

ReconstructDecoderTree PROC C
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    sub esp,8
    xor edi,edi
    xor ecx,ecx
    jmp L_7ff1c
L_7ff1b:
    inc ecx
L_7ff1c:
    movsx esi,cx
    cmp esi,627
    jge L_7ff69
    add esi,esi
    mov eax,DWORD PTR [esi+son-2]
    sar eax,16
    cmp eax,627
    jl L_7ff1b
    movzx edx,WORD PTR [esi+freq]
    inc edx
    mov eax,edx
    sar edx,31
    sub eax,edx
    sar eax,1
    mov edx,eax
    movsx eax,di
    mov WORD PTR [eax*2+freq],dx
    mov dx,WORD PTR [esi+son]
    inc edi
    mov WORD PTR [eax*2+son],dx
    jmp L_7ff1b
L_7ff69:
    xor edx,edx
    mov edi,314
    mov WORD PTR [esp+4],dx
    jmp L_7ffd5
L_7ff77:
    inc eax
    mov edx,edi
    sub edx,eax
    movsx esi,ax
    add edx,edx
    add esi,esi
    movzx ebp,dx
    mov edx,OFFSET freq
    add edx,esi
    lea eax,[esi+2]
    mov DWORD PTR [esp],eax
    mov eax,OFFSET freq
    add eax,DWORD PTR [esp]
    mov ebx,ebp
    call LzhufMemmove
    mov edx,OFFSET son
    mov eax,OFFSET son
    mov ebx,ebp
    mov WORD PTR [esi+freq],cx
    mov ecx,DWORD PTR [esp]
    add edx,esi
    add eax,ecx
    call LzhufMemmove
    mov eax,DWORD PTR [esp+4]
    mov WORD PTR [esi+son],ax
    add eax,2
    inc edi
    mov WORD PTR [esp+4],ax
L_7ffd5:
    movsx ecx,di
    cmp ecx,627
    jge L_80018
    mov edx,DWORD PTR [esp+2]
    sar edx,16
    mov ax,WORD PTR [edx*2+freq]
    mov bx,WORD PTR [edx*2+freq+2]
    add eax,ebx
    mov WORD PTR [ecx*2+freq],ax
    mov ecx,eax
    mov eax,edi
L_80005:
    dec eax
    movsx edx,ax
    cmp cx,WORD PTR [edx*2+freq]
    jb L_80005
    jmp L_7ff77
L_80018:
    xor ecx,ecx
    jmp L_8002b
L_8001c:
    mov WORD PTR [edx+prnt+2],cx
    mov WORD PTR [edx+prnt],cx
L_8002a:
    inc ecx
L_8002b:
    movsx eax,cx
    cmp eax,627
    jge L_80055
    mov ax,WORD PTR [eax*2+son]
    cwde
    lea edx,[eax*2+0]
    cmp eax,627
    jl L_8001c
    mov WORD PTR [edx+prnt],cx
    jmp L_8002a
L_80055:
    add esp,8
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    ret
ReconstructDecoderTree ENDP

END
