// Processor detection and speed estimate. The bodies are C functions around
// inline-assembly blocks (retail keeps the C frame, the short result local and
// the epilogue jump); SetGameDefaults picks the walk speed from GetCPUType.

#include <match.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

#include <SOURCE/kbwin.h>

// The 800-instruction divide loop TimeProcessor clocks against PIT channel 2:
// each DIV_BX is one 16-bit "div bx" (DX:AX / BX).
#define DIV_BX __asm div bx
#define DIV_BX_10 DIV_BX DIV_BX DIV_BX DIV_BX DIV_BX DIV_BX DIV_BX DIV_BX DIV_BX DIV_BX
#define DIV_BX_100                                                                                 \
    DIV_BX_10 DIV_BX_10 DIV_BX_10 DIV_BX_10 DIV_BX_10 DIV_BX_10 DIV_BX_10 DIV_BX_10 DIV_BX_10      \
        DIV_BX_10

// Expected PIT ticks for 800 divides at the family's reference clock, scaled
// by the measured ticks to MHz.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0043c840, 0x145)
i32 CPUSpeed(u8 cpuType) {
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
    return static_cast<i32>((freq + 0.5) * 100.0) / 100;
}

// Win95 1.2 asks Windows for the processor family; unknown types return zero.
VA(0x0043c985, 0x87)
i16 GetCPUType(void) {
    SYSTEM_INFO info;
    memset(&info, 0, sizeof(info));
    GetSystemInfo(&info);
    switch (info.dwProcessorType) {
        case PROCESSOR_INTEL_386: return CPU_FAMILY_386;
        case PROCESSOR_INTEL_486: return CPU_FAMILY_486;
        case PROCESSOR_INTEL_PENTIUM: return CPU_FAMILY_PENTIUM;
    }
    return 0;
}

// Counts PIT channel-2 ticks across the divide loop with the speaker gate
// raised and NMI masked.
// What the assembly does, in C++ (jmp-short pairs are I/O delays):
//
//   outp(0x43, 0xb0);                    // channel 2, lo/hi byte, mode 0
//   outp(0x42, 0xff); outp(0x42, 0xff);  // count 0xffff
//   _disable(); outp(0x70, 0x80);        // mask interrupts and NMI
//   unsigned short ax = inp(0x61) | 1;   // gate channel 2 on
//   outp(0x61, ax);
//   for (int i = 0; i < 800; ++i) ax = (unsigned short)(((unsigned long)0 << 16 | ax) / 1);
//                                        // DIV_BX_100 x 8: 800 "div bx", DX = 0, BX = 1
//   outp(0x61, inp(0x61) & 0xfe);        // gate off
//   outp(0x70, 0); _enable();            // unmask NMI and interrupts
//   outp(0x43, 0x80);                    // latch channel 2
//   unsigned short left = inp(0x42); left |= inp(0x42) << 8;
//   return (short)~left;                 // ticks elapsed from 0xffff
VA(0x0043ca0c, 0x9d9)
i16 TimeProcessor(void) {
    i16 ticks;

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
