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
EXTERN UpdateDecoderTree:PROC
EXTERN DecodePosition:PROC

.code

Decode PROC C
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    sub esp,12
    mov esi,DWORD PTR cs:[decodeOutput]
    mov ebp,DWORD PTR cs:[decodeSize]
    mov edx,4036
    xor ecx,ecx
L_8007e:
    mov eax,textsize
    add eax,ebp
    cmp ecx,eax
    jae L_8017b
    mov bx,WORD PTR son+1252
L_80094:
    movzx eax,bx
    cmp eax,627
    jge L_800b5
    call GetBit
    add ebx,eax
    and ebx,65535
    mov bx,WORD PTR [ebx*2+son]
    jmp L_80094
L_800b5:
    sub ebx,627
    movsx edi,bx
    mov eax,edi
    mov WORD PTR [esp+4],bx
    call UpdateDecoderTree
    cmp edi,256
    jge L_800f6
    cmp ecx,DWORD PTR textsize
    jb L_800e2
    inc esi
    mov al,BYTE PTR [esp+4]
    mov BYTE PTR [esi-1],al
L_800e2:
    mov bl,BYTE PTR [esp+4]
    movsx eax,dx
    inc ecx
    inc edx
    mov BYTE PTR [eax+text_buf],bl
    and dh,15
    jmp L_8007e
L_800f6:
    mov edi,edx
    call DecodePosition
    sub edi,eax
    sub ebx,253
    mov eax,edi
    mov DWORD PTR [esp],ebx
    dec eax
    xor ebx,ebx
    and ah,15
    mov edi,DWORD PTR [esp]
    mov WORD PTR [esp+8],ax
    test di,di
    jle L_8007e
    jmp L_80142
L_80123:
    mov al,BYTE PTR [esp+4]
    movsx edi,dx
    inc ecx
    inc ebx
    inc edx
    mov BYTE PTR [edi+text_buf],al
    mov eax,DWORD PTR [esp]
    and dh,15
    cmp bx,ax
    jge L_8007e
L_80142:
    mov eax,DWORD PTR [esp+6]
    movsx edi,bx
    sar eax,16
    add eax,edi
    and eax,4095
    mov al,BYTE PTR [eax+text_buf]
    xor ah,ah
    mov edi,DWORD PTR textsize
    mov WORD PTR [esp+4],ax
    cmp ecx,edi
    jb L_80123
    lea eax,[edi+ebp*1]
    cmp ecx,eax
    jae L_80123
    inc esi
    mov al,BYTE PTR [esp+4]
    mov BYTE PTR [esi-1],al
    jmp L_80123
L_8017b:
    mov eax,ecx
    add esp,12
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret
Decode ENDP

END
