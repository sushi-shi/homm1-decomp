#ifndef HOMM1_DOMAINS_H
#define HOMM1_DOMAINS_H

// Adapted from HoMM2's Ints.h: one source, strict domains in analysis,
// explicitly chosen integer representations for the retail compiler.
#if defined(__cplusplus) && __cplusplus >= 202002L
template <typename Domain, typename Storage> class H1EnumStorage {
public:
    H1EnumStorage() = default;
    constexpr H1EnumStorage(Domain value) : value_(static_cast<Storage>(value)) {}
    constexpr operator Domain() const { return static_cast<Domain>(value_); }
    H1EnumStorage& operator=(Domain value) {
        value_ = static_cast<Storage>(value);
        return *this;
    }
private:
    Storage value_;
};
#define H1_ENUM_BEGIN(name) enum class name : int {
#define H1_ENUM_END(name) }; using enum name;
#define H1_ENUM_PARAM(name, storage) name
#define H1_ENUM_RETURN(name, storage) name
#define H1_ENUM_STORAGE(name, storage) H1EnumStorage<name, storage>
#define H1_ENUM_CAST(name, storage, value) static_cast<name>(value)
#else
#define H1_ENUM_BEGIN(name) enum name {
#define H1_ENUM_END(name) };
#define H1_ENUM_PARAM(name, storage) storage
#define H1_ENUM_RETURN(name, storage) storage
#define H1_ENUM_STORAGE(name, storage) storage
#define H1_ENUM_CAST(name, storage, value) static_cast<storage>(value)
#endif

#endif
