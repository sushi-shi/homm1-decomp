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
#endif

#endif
