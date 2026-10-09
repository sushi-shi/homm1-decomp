.386
.model flat, C
option casemap:none
option prologue:none
option epilogue:none

EXTERN d_code:BYTE
EXTERN d_len:BYTE
EXTERN dad:WORD
EXTERN decodeLen:DWORD
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
EXTERN dataPtr:DWORD
EXTERN match_position:WORD
EXTERN match_length:WORD
EXTERN outputPos:DWORD

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
    db 08eh, 0c2h
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
    db 08eh, 0c2h
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
    mov ebx,DWORD PTR dataPtr
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
    mov DWORD PTR dataPtr,ebx
    pop esi
    pop edx
    pop ecx
    pop ebx
    ret
GetBit ENDP

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
    mov ebx,DWORD PTR dataPtr
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
    mov DWORD PTR dataPtr,ebx
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
    cmp dx,WORD PTR 65535
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

UpdateDecoderTree PROC C
    push edx
    push ebx
    push ecx
    push esi
    push edi
    mov esi,eax
    xor edx,edx
    mov dx,WORD PTR freq+1252
    cmp edx,32768
    jne L_7fe10
    call ReconstructDecoderTree
L_7fe10:
    movsx esi,si
    mov si,WORD PTR [esi*2+prnt+1254]
L_7fe1b:
    movsx ecx,si
    mov dx,WORD PTR [ecx*2+freq]
    mov eax,esi
    inc edx
    inc eax
    mov WORD PTR [ecx*2+freq],dx
    mov ecx,edx
    movsx edx,ax
    mov dx,WORD PTR [edx*2+freq]
    and edx,65535
    movsx ebx,cx
    cmp ebx,edx
    jle L_7fef0
    movsx ebx,cx
L_7fe53:
    inc eax
    movsx edx,ax
    mov dx,WORD PTR [edx*2+freq]
    and edx,65535
    cmp ebx,edx
    jg L_7fe53
    dec eax
    movsx ebx,ax
    movsx edx,si
    mov di,WORD PTR [ebx*2+freq]
    mov WORD PTR [edx*2+freq],di
    mov dx,WORD PTR [edx*2+son]
    mov WORD PTR [ebx*2+freq],cx
    movsx ecx,dx
    lea ebx,[ecx*2+0]
    mov WORD PTR [ebx+prnt],ax
    cmp ecx,627
    jge L_7feb0
    mov WORD PTR [ebx+prnt+2],ax
L_7feb0:
    movsx ecx,ax
    mov bx,WORD PTR [ecx*2+son]
    mov WORD PTR [ecx*2+son],dx
    movsx edx,bx
    lea ecx,[edx*2+0]
    mov WORD PTR [ecx+prnt],si
    cmp edx,627
    jge L_7fee3
    mov WORD PTR [ecx+prnt+2],si
L_7fee3:
    movsx edx,si
    mov esi,eax
    mov WORD PTR [edx*2+son],bx
L_7fef0:
    movsx esi,si
    mov si,WORD PTR [esi*2+prnt]
    movsx edx,si
    test edx,edx
    jne L_7fe1b
    pop edi
    pop esi
    pop ecx
    pop ebx
    pop edx
    ret
UpdateDecoderTree ENDP

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

Decode PROC C
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    sub esp,12
    mov esi,DWORD PTR cs:[outputPos]
    mov ebp,DWORD PTR cs:[decodeLen]
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
