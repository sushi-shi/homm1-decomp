// Processor detection and speed estimate. The bodies are C functions around
// inline-assembly blocks (retail keeps the C frame, the short result local and
// the epilogue jump); SetGameDefaults picks the walk speed from GetCPUType.

#include <match.h>

#include <SOURCE/kbwin.h>

// The 800-instruction divide loop TimeProcessor clocks against PIT channel 2.
#define DIV_BX __asm div bx
#define DIV_BX_10 DIV_BX DIV_BX DIV_BX DIV_BX DIV_BX DIV_BX DIV_BX DIV_BX DIV_BX DIV_BX
#define DIV_BX_100                                                                                 \
    DIV_BX_10 DIV_BX_10 DIV_BX_10 DIV_BX_10 DIV_BX_10 DIV_BX_10 DIV_BX_10 DIV_BX_10 DIV_BX_10      \
        DIV_BX_10

// Expected PIT ticks for 800 divides at the family's reference clock, scaled
// by the measured ticks to MHz.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00472030, 0x1c9)
int CPUSpeed(unsigned char cpuType) {
    double tickPeriod = 838.0965152;
    double divs = 800.0;
    double ticks;
    double clockNs;
    double freq;
    double totalNs;
    double divNs;

    switch (cpuType) {
        case CPU_FAMILY_386:
            clockNs = 62.5;
            ticks = (totalNs = (divNs = clockNs * 22.0) * divs) / tickPeriod;
            freq = ticks / TimeProcessor() * 16.0;
            break;
        case CPU_FAMILY_486:
            clockNs = 30.303030303030305;
            ticks = (totalNs = (divNs = clockNs * 24.0) * divs) / tickPeriod;
            freq = ticks / TimeProcessor() * 33.0;
            break;
        default:
            clockNs = 15.151515151515152;
            ticks = (totalNs = (divNs = clockNs * 25.0) * divs) / tickPeriod;
            freq = ticks / TimeProcessor() * 66.0;
            break;
    }
    return static_cast<int>((freq + 0.5) * 100.0) / 100;
}

// Family 3 when EFLAGS.AC cannot toggle, 4 when EFLAGS.ID cannot toggle,
// otherwise the CPUID family with 1 in the high byte.
VA(0x004721f9, 0x80)
short GetCPUType(void) {
    short cpuType;

    __asm {
        push eax
        push ebx
        push ecx
        push edx
        push ds
        push es
        cli
        pushfd
        pop eax
        mov ebx, eax
        xor eax, 40000h
        push eax
        popfd
        pushfd
        pop eax
        push ebx
        popfd
        sti
        xor eax, ebx
        jnz check_486
        mov ax, 3
        jmp done
    check_486:
        cli
        pushfd
        pop eax
        mov ebx, eax
        xor eax, 200000h
        push eax
        popfd
        pushfd
        pop eax
        push ebx
        popfd
        sti
        xor eax, ebx
        jnz has_cpuid
        mov ax, 4
        jmp done
    has_cpuid:
        push ecx
        push edx
        mov eax, 1
        _emit 0x0f
        _emit 0xa2
        and eax, 0f00h
        shr eax, 8
        mov ah, 1
        mov cpuType, ax
        pop edx
        pop ecx
    done:
        pop es
        pop ds
        pop edx
        pop ecx
        pop ebx
        pop eax
    }
    return cpuType;
}

// Counts PIT channel-2 ticks across the divide loop with the speaker gate
// raised and NMI masked.
VA(0x00472279, 0x9d9)
short TimeProcessor(void) {
    short ticks;

    __asm {
        push eax
        push ebx
        push ecx
        push edx
        mov al, 0b0h
        out 43h, al
        jmp short delay1
    delay1:
        jmp short delay2
    delay2:
        mov al, 0ffh
        out 42h, al
        jmp short delay3
    delay3:
        jmp short delay4
    delay4:
        out 42h, al
        jmp short delay5
    delay5:
        jmp short delay6
    delay6:
        cli
        mov al, 80h
        out 70h, al
        jmp short delay7
    delay7:
        jmp short delay8
    delay8:
        in al, 61h
        jmp short delay9
    delay9:
        jmp short delay10
    delay10:
        xor dx, dx
        mov bx, 1
        or al, 1
        out 61h, al
    }
    DIV_BX_100
    DIV_BX_100
    DIV_BX_100
    DIV_BX_100
    DIV_BX_100
    DIV_BX_100
    DIV_BX_100
    DIV_BX_100
    __asm {
        in al, 61h
        jmp short delay11
    delay11:
        jmp short delay12
    delay12:
        and al, 0feh
        out 61h, al
        xor al, al
        out 70h, al
        sti
        mov al, 80h
        out 43h, al
        jmp short delay13
    delay13:
        jmp short delay14
    delay14:
        in al, 42h
        jmp short delay15
    delay15:
        jmp short delay16
    delay16:
        mov dl, al
        in al, 42h
        mov dh, al
        not dx
        mov ticks, dx
        pop edx
        pop ecx
        pop ebx
        pop eax
    }
    return ticks;
}
