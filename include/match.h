#ifndef HOMM1_MATCH_H
#define HOMM1_MATCH_H

// Fixed-width integer aliases (H1/Ints.h). Every translation unit opens this
// header first, so defining them here makes them reachable everywhere without
// opening another header.
#ifndef HOMM1_INTS_DEFINED
#define HOMM1_INTS_DEFINED
typedef signed char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef __int64 i64;
typedef unsigned __int64 u64;
#endif

// Boolean storage: b8 and b32 hold a flag in an 8- or 32-bit integer, bchar
// in a plain char, and are written with true/false. The retail view is the
// plain integer, so loads, stores and tests compile exactly as before (C++
// bool would normalize stores). The strict C++20 view wraps the integer in
// H1Bool, which converts to and from bool only: assigning or comparing an
// integer is an error.
#ifndef HOMM1_BOOL_DEFINED
#define HOMM1_BOOL_DEFINED
#if defined(__cplusplus) && __cplusplus >= 202002L
template<typename Storage> class H1Bool;
template<typename T> constexpr bool H1IsBool = __is_same(T, bool);
template<typename S> constexpr bool H1IsBool<H1Bool<S> > = true;
template<typename Storage> class H1Bool {
public:
    H1Bool() = default;
    constexpr H1Bool(bool value) : value_(value) {}
    // A flag of the other width is still a flag (the retail store narrows
    // or widens the 0/1 integer).
    template<typename Other>
    constexpr H1Bool(H1Bool<Other> other) : value_(static_cast<bool>(other)) {}
    template<typename Other> requires(!H1IsBool<Other>) H1Bool(Other) = delete;
    constexpr operator bool() const {
        return value_ != 0;
    }

private:
    Storage value_;
};
template<typename Storage, typename Other>
requires(!H1IsBool<Other>) bool operator==(H1Bool<Storage>, Other) = delete;
typedef H1Bool<i8> b8;
typedef H1Bool<i32> b32;
typedef H1Bool<char> bchar;
#else
typedef i8 b8;
typedef i32 b32;
typedef char bchar;
#endif
#endif

// Buka's project-wide prelude: every retail C++ unit (sixty /Od and two /O2)
// instantiates std::ctype<wchar_t>::id and registers its cleanup, the output
// of <string> (docs/patterns/vc6-ctype-startup.md).
#include <string>

// Reconstruction metadata. The compiler receives ordinary C++.
// A claim spells an address of the program its source tree belongs to
// (src/EDITOR and include/EDITOR: EDITOR.EXE; elsewhere: HEROES.EXE).
// VA_AT names another program's body of the same function, for source one
// program compiles differently from the other (`image` is a targets.json key).
#ifdef __clang__
#define VA(address, size) __attribute__((annotate("va:" #address " size:" #size), used))
#define VA_AT(image, address, size)                                                                \
    __attribute__((annotate("va:" #address " size:" #size " image:" #image), used))
#define VA_DECL(address) __attribute__((annotate("decl-va:" #address)))
#define DATA(address) __attribute__((annotate("data-va:" #address), used))
#else
#define VA(address, size)
#define VA_AT(image, address, size)
#define VA_DECL(address)
#define DATA(address)
#endif

// Generated code has explicit retail identity and an owning source VA.
// The compiler itself supplies the body; these are not C++ implementations.
#define VA_COMPGEN(address, size, symbol, owner)
// A file-scope object's compiler-emitted dynamic initializer (_$E<n>): its
// retail VA and size, pinned to the owning datum.
#define RVA_DYNINIT(address, size, owner)

#endif // HOMM1_MATCH_H
