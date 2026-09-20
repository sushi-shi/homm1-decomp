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

EXTERN ReconstructDecoderTree:PROC

.code

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

END
