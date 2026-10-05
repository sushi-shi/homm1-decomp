#ifndef HOMM1_DOMAINS_H
#define HOMM1_DOMAINS_H

// One source, strict domains in analysis, explicitly chosen integer
// representations for the retail compiler.
//
// BEGIN_SPLIT declares a domain stored at one retail width, FLAGS_BEGIN a bit
// set whose members combine with `|`, and CONST_BEGIN a group of named
// encoding biases, masks, sentinels and extents that is not a value domain.
// The retail spelling of every form is the same named `enum name {`.
// STORAGE types a field or global at the domain's width; PARAM, RETURN and
// LOCAL type a parameter, return value or temporary at its own retail width.
//
// ENUM_ARRAY(type, name, Domain, count) declares an array indexed by one
// domain: the retail view is the plain `type name[count]` (no class, no
// inline operator[], so VC6 frames and C1 state are unchanged); the strict
// view is an H1EnumArray whose subscript accepts only Domain (or its typed
// storage), so a raw integer or another domain's value is a strict-view
// error. ENUM_ARRAY2 nests two domain-indexed dimensions. A dimension with no
// domain stays a plain array. STEPPED(Domain) gives a stepped domain (a loop
// variable or a `+ 1` to the next member) its increment and offset operators
// in the strict view; the retail view has none.
#if defined(__cplusplus) && __cplusplus >= 202002L
template<typename Domain, typename Storage> class H1EnumStorage {
public:
    H1EnumStorage() = default;
    constexpr H1EnumStorage(Domain value) : value_(static_cast<Storage>(value)) {}
    constexpr operator Domain() const {
        return static_cast<Domain>(value_);
    }
    H1EnumStorage& operator=(Domain value) {
        value_ = static_cast<Storage>(value);
        return *this;
    }

private:
    Storage value_;
};
template<typename T, typename Domain, int Count> class H1EnumArray {
public:
    T elements[Count];

    constexpr T& operator[](Domain index) {
        return elements[static_cast<int>(index)];
    }
    constexpr const T& operator[](Domain index) const {
        return elements[static_cast<int>(index)];
    }
    // An integer, another domain or anything not convertible to Domain.
    template<typename Index>
        requires(!__is_convertible_to(Index, Domain))
    T& operator[](Index) = delete;
    template<typename Index>
        requires(!__is_convertible_to(Index, Domain))
    const T& operator[](Index) const = delete;

    // The array's base and an element address, as the retail array decays.
    constexpr operator T*() {
        return elements;
    }
    constexpr operator const T*() const {
        return elements;
    }
    constexpr T* operator+(Domain index) {
        return elements + static_cast<int>(index);
    }
    constexpr const T* operator+(Domain index) const {
        return elements + static_cast<int>(index);
    }
    template<typename Index>
        requires(!__is_convertible_to(Index, Domain))
    T* operator+(Index) = delete;
};
#define H1_ENUM_ARRAY(type, name, domain, count)                                                   \
    H1EnumArray<type, domain, static_cast<int>(count)> name
#define H1_ENUM_ARRAY2(type, name, domain1, count1, domain2, count2)                               \
    H1EnumArray<H1EnumArray<type, domain2, static_cast<int>(count2)>, domain1,                     \
                static_cast<int>(count1)>                                                          \
        name
#define H1_ENUM_STEPPED(name)                                                                      \
    inline constexpr name operator+(name a, int amount) {                                          \
        return static_cast<name>(static_cast<int>(a) + amount);                                    \
    }                                                                                              \
    inline constexpr name operator-(name a, int amount) {                                          \
        return static_cast<name>(static_cast<int>(a) - amount);                                    \
    }                                                                                              \
    inline constexpr int operator-(name a, name b) {                                               \
        return static_cast<int>(a) - static_cast<int>(b);                                          \
    }                                                                                              \
    inline name& operator+=(name& a, int amount) {                                                 \
        return a = a + amount;                                                                     \
    }                                                                                              \
    inline name& operator-=(name& a, int amount) {                                                 \
        return a = a - amount;                                                                     \
    }                                                                                              \
    inline name& operator++(name& a) {                                                             \
        return a = a + 1;                                                                          \
    }                                                                                              \
    inline name operator++(name& a, int) {                                                         \
        name old = a;                                                                              \
        ++a;                                                                                       \
        return old;                                                                                \
    }                                                                                              \
    inline name& operator--(name& a) {                                                             \
        return a = a - 1;                                                                          \
    }                                                                                              \
    inline name operator--(name& a, int) {                                                         \
        name old = a;                                                                              \
        --a;                                                                                       \
        return old;                                                                                \
    }
#define H1_ENUM_BEGIN(name) enum class name : int {
#define H1_ENUM_END(name)                                                                          \
    }                                                                                              \
    ;                                                                                              \
    using enum name;
#define H1_ENUM_PARAM(name, storage) name
#define H1_ENUM_RETURN(name, storage) name
#define H1_ENUM_STORAGE(name, storage) H1EnumStorage<name, storage>
#define H1_ENUM_CAST(name, storage, value) static_cast<name>(value)
#define H1_ENUM_BEGIN_SPLIT(name, storage) enum class name : int {
#define H1_ENUM_END_SPLIT(name)                                                                    \
    }                                                                                              \
    ;                                                                                              \
    using enum name;
#define H1_ENUM_FLAGS_BEGIN(name, storage) enum name : int {
#define H1_ENUM_FLAGS_END(name)                                                                    \
    }                                                                                              \
    ;
#define H1_ENUM_CONST_BEGIN(name) enum name : int {
#define H1_ENUM_CONST_END(name)                                                                    \
    }                                                                                              \
    ;
#define H1_ENUM_LOCAL(name, storage) name
#else
#define H1_ENUM_BEGIN(name) enum name {
#define H1_ENUM_END(name)                                                                          \
    }                                                                                              \
    ;
#define H1_ENUM_PARAM(name, storage) storage
#define H1_ENUM_RETURN(name, storage) storage
#define H1_ENUM_STORAGE(name, storage) storage
#define H1_ENUM_CAST(name, storage, value) static_cast<storage>(value)
#define H1_ENUM_BEGIN_SPLIT(name, storage) enum name {
#define H1_ENUM_END_SPLIT(name)                                                                    \
    }                                                                                              \
    ;
#define H1_ENUM_FLAGS_BEGIN(name, storage) enum name {
#define H1_ENUM_FLAGS_END(name)                                                                    \
    }                                                                                              \
    ;
#define H1_ENUM_CONST_BEGIN(name) enum name {
#define H1_ENUM_CONST_END(name)                                                                    \
    }                                                                                              \
    ;
#define H1_ENUM_LOCAL(name, storage) storage
#define H1_ENUM_ARRAY(type, name, domain, count) type name[count]
#define H1_ENUM_ARRAY2(type, name, domain1, count1, domain2, count2) type name[count1][count2]
#define H1_ENUM_STEPPED(name)
#endif

#endif
