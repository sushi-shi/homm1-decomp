; Retail HoMM1 and both HoMM2 donor builds use this MASM translation unit.

.386
.model flat, C
option casemap:none
option prologue:none
option epilogue:none

.code

; Bit arrays addressed LSB-first within each byte (include/BASE/BITS.h,
; extern "C" __cdecl). Each routine reads/writes the DWORD at byte bit >> 3;
; the mask is below 0x100, so only that byte can change.

; extern "C" int __cdecl BitTest(const void* bits, unsigned int bit) {
;     return (((const unsigned char*)bits)[bit >> 3] & (1 << (bit & 7))) ? 1 : 0;
; }
BitTest PROC C
    push ebp
    mov ebp, esp
    push esi
    mov esi, DWORD PTR [ebp+8]
    mov eax, DWORD PTR [ebp+12]
    mov ecx, eax
    shr eax, 3
    and ecx, 7
    add esi, eax
    mov eax, 1
    shl eax, cl
    and eax, DWORD PTR [esi]
    jne bit_test_set
    mov eax, 0
    jmp bit_test_done
bit_test_set:
    mov eax, 1
bit_test_done:
    pop esi
    pop ebp
    ret
BitTest ENDP

; extern "C" void __cdecl BitSet(void* bits, unsigned int bit) {
;     ((unsigned char*)bits)[bit >> 3] |= 1 << (bit & 7);
; }
BitSet PROC C
    push ebp
    mov ebp, esp
    push esi
    mov esi, DWORD PTR [ebp+8]
    mov eax, DWORD PTR [ebp+12]
    mov ecx, eax
    shr eax, 3
    and ecx, 7
    add esi, eax
    mov eax, 1
    shl eax, cl
    or DWORD PTR [esi], eax
    pop esi
    pop ebp
    ret
BitSet ENDP

; @dead-code
; Zero-ref: no effective incoming retail reference; both HoMM2 donors retain it.
; extern "C" void __cdecl BitClear(void* bits, unsigned int bit) {
;     ((unsigned char*)bits)[bit >> 3] &= ~(1 << (bit & 7));
; }
BitClear PROC C
    push ebp
    mov ebp, esp
    push esi
    mov esi, DWORD PTR [ebp+8]
    mov eax, DWORD PTR [ebp+12]
    mov ecx, eax
    shr eax, 3
    and ecx, 7
    add esi, eax
    mov eax, 1
    shl eax, cl
    not eax
    and DWORD PTR [esi], eax
    pop esi
    pop ebp
    ret
BitClear ENDP

END
