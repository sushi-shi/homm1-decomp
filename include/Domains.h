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
// error. ENUM_ARRAY2 nests two domain-indexed dimensions; ENUM_ARRAY_ROWS
// indexes rows of `columns` plain elements by the domain. A dimension with no
// domain otherwise stays a plain array (`ENUM_ARRAY(T, name[N], D, COUNT)`
// is N rows indexed by D). STEPPED(Domain) gives a stepped domain (a loop
// variable or a `+ 1` to the next member) its increment and offset operators
// in the strict view; the retail view has none. H1_ENUM_BIT(Domain, value)
// is `1 << value` for a mask over a domain. H1_ENUM_SHARED(Domain, storage)
// declares storage the retail program reuses for a domain value and another
// integer (its comment names the second use). H1_ENUM_DECODE/ENCODE convert at
// integer storage the retail program shares between a domain and other
// integers; the storage's comment names the encoding.
//
// ID_BEGIN declares a set of codes carried by a shared integer transport (a
// window's widget ids in tag_message::id and m_dialogResult, Win32 command
// ids). The strict view is an unscoped `enum name : int`: an id converts to
// the transport's integer, but no integer or other set converts to an id.
// A FLAGS set keeps its type through `|`, `&`, `^` and `~` in the strict view.
// H1_STRICT_DOMAINS is 1 in the strict view: a header that packs a domain
// into a raw byte spells its encode/decode helpers once per view.
#if defined(__cplusplus) && __cplusplus >= 202002L
#define H1_STRICT_DOMAINS 1
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
    // A flag set (an unscoped FLAGS domain) combines in place.
    H1EnumStorage& operator|=(Domain value) requires(__is_convertible_to(Domain, int)) {
        value_ = static_cast<Storage>(value_ | static_cast<int>(value));
        return *this;
    }
    H1EnumStorage& operator&=(Domain value) requires(__is_convertible_to(Domain, int)) {
        value_ = static_cast<Storage>(value_ & static_cast<int>(value));
        return *this;
    }
    H1EnumStorage& operator^=(Domain value) requires(__is_convertible_to(Domain, int)) {
        value_ = static_cast<Storage>(value_ ^ static_cast<int>(value));
        return *this;
    }
    // A STEPPED domain steps in place.
    H1EnumStorage& operator+=(int amount) requires requires(Domain d) {
        d + 1;
    }
    {
        return *this = static_cast<Domain>(*this) + amount;
    }
    H1EnumStorage& operator-=(int amount) requires requires(Domain d) {
        d - 1;
    }
    {
        return *this = static_cast<Domain>(*this) - amount;
    }
    H1EnumStorage& operator++() requires requires(Domain d) {
        d + 1;
    }
    {
        return *this += 1;
    }
    Domain operator++(int postfix) requires requires(Domain d) {
        d + 1;
    }
    {
        Domain old = *this;
        *this += 1;
        return old;
    }
    H1EnumStorage& operator--() requires requires(Domain d) {
        d - 1;
    }
    {
        return *this -= 1;
    }
    Domain operator--(int postfix) requires requires(Domain d) {
        d - 1;
    }
    {
        Domain old = *this;
        *this -= 1;
        return old;
    }

private:
    Storage value_;
};
// A local or field the retail program uses both for a domain value and for
// another integer (one stack slot reused by two loops, a byte that holds a
// value or a count): it compares and converts as either face.
template<typename Domain, typename Storage> class H1EnumShared {
public:
    H1EnumShared() = default;
    constexpr H1EnumShared(Domain value) : value_(static_cast<Storage>(value)) {}
    template<typename Value>
    requires(__is_integral(Value)) constexpr H1EnumShared(Value value)
        : value_(static_cast<Storage>(value)) {}
    constexpr operator Domain() const {
        return static_cast<Domain>(value_);
    }
    constexpr operator Storage() const {
        return value_;
    }
    // Offsets of the integer face: `shared + n` is a number, whatever the
    // domain steps.
    constexpr Storage operator+(int amount) const {
        return static_cast<Storage>(value_ + amount);
    }
    constexpr Storage operator-(int amount) const {
        return static_cast<Storage>(value_ - amount);
    }
    H1EnumShared& operator+=(int amount) {
        value_ = static_cast<Storage>(value_ + amount);
        return *this;
    }
    H1EnumShared& operator-=(int amount) {
        value_ = static_cast<Storage>(value_ - amount);
        return *this;
    }
    H1EnumShared& operator++() {
        return *this += 1;
    }
    Storage operator++(int postfix) {
        Storage old = value_;
        *this += 1;
        return old;
    }
    H1EnumShared& operator--() {
        return *this -= 1;
    }
    Storage operator--(int postfix) {
        Storage old = value_;
        *this -= 1;
        return old;
    }

private:
    Storage value_;
};
// A braced list of whole elements initializes an H1EnumArray (a struct per
// row, a row of columns per element) as it initializes the retail array; the
// strict view declares the list type clang's list-initialization uses.
namespace std {
    template<typename Element> class initializer_list {
        typedef decltype(sizeof(0)) Size;
        const Element* first_;
        Size size_;
        constexpr initializer_list(const Element* first, Size size) : first_(first), size_(size) {}

    public:
        constexpr initializer_list() : first_(0), size_(0) {}
        constexpr Size size() const {
            return size_;
        }
        constexpr const Element* begin() const {
            return first_;
        }
        constexpr const Element* end() const {
            return first_ + size_;
        }
    };
} // namespace std
template<typename T> constexpr void H1EnumArrayCopy(T& to, const T& from) {
    to = from;
}
template<typename T, int N> constexpr void H1EnumArrayCopy(T (&to)[N], const T (&from)[N]) {
    for (int i = 0; i < N; i++)
        H1EnumArrayCopy(to[i], from[i]);
}
template<typename T, typename Domain, int Count> class H1EnumArray {
public:
    T elements[Count];

    H1EnumArray() = default;
    // Elements past the list are zero, as in a retail array initializer.
    constexpr H1EnumArray(std::initializer_list<T> list) : elements{} {
        int index = 0;
        for (const T* element = list.begin(); element != list.end() && index < Count;
             ++element, ++index)
            H1EnumArrayCopy(elements[index], *element);
    }

    constexpr T& operator[](Domain index) {
        return elements[static_cast<int>(index)];
    }
    constexpr const T& operator[](Domain index) const {
        return elements[static_cast<int>(index)];
    }
    // An integer, another domain or anything not convertible to Domain.
    template<typename Index>
    requires(!__is_convertible_to(Index, Domain)) T& operator[](Index) = delete;
    template<typename Index>
    requires(!__is_convertible_to(Index, Domain)) const T& operator[](Index) const = delete;

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
    requires(!__is_convertible_to(Index, Domain)) T* operator+(Index) = delete;
};
#define H1_ENUM_ARRAY(type, name, domain, count)                                                   \
    H1EnumArray<type, domain, static_cast<int>(count)> name
#define H1_ENUM_ARRAY_ROWS(type, name, domain, count, columns)                                     \
    H1EnumArray<type[columns], domain, static_cast<int>(count)> name
#define H1_ENUM_ARRAY2(type, name, domain1, count1, domain2, count2)                               \
    H1EnumArray<                                                                                   \
        H1EnumArray<type, domain2, static_cast<int>(count2)>,                                      \
        domain1,                                                                                   \
        static_cast<int>(count1)>                                                                  \
        name
// The bit of a domain value in a mask over that domain: the strict view
// accepts only a value of Domain.
template<typename Domain> constexpr int H1EnumBit(Domain value) {
    return 1 << static_cast<int>(value);
}
#define H1_ENUM_BIT(domain, value) H1EnumBit<domain>(value)
// A domain value read from or written to integer storage the retail program
// shares with other integers: a local reused for a second quantity, a byte
// that packs the value with a flag or offset, a table cell carrying several
// encodings. DECODE accepts only an integer, ENCODE only a value of Domain.
template<typename Domain, typename Value>
requires(__is_integral(Value)) constexpr Domain H1EnumDecode(Value value) {
    return static_cast<Domain>(value);
}
template<typename Domain> constexpr int H1EnumEncode(Domain value) {
    return static_cast<int>(value);
}
#define H1_ENUM_DECODE(domain, value) H1EnumDecode<domain>(value)
#define H1_ENUM_ENCODE(domain, value) H1EnumEncode<domain>(value)
#define H1_ENUM_STEPPED(name)                                                                      \
    inline constexpr name operator+(name a, int amount) {                                          \
        return static_cast<name>(static_cast<int>(a) + amount);                                    \
    }                                                                                              \
    inline constexpr name operator+(int amount, name a) {                                          \
        return static_cast<name>(amount + static_cast<int>(a));                                    \
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
    ;                                                                                              \
    inline constexpr name operator|(name a, name b) {                                              \
        return static_cast<name>(static_cast<int>(a) | static_cast<int>(b));                       \
    }                                                                                              \
    inline constexpr name operator&(name a, name b) {                                              \
        return static_cast<name>(static_cast<int>(a) & static_cast<int>(b));                       \
    }                                                                                              \
    inline constexpr name operator^(name a, name b) {                                              \
        return static_cast<name>(static_cast<int>(a) ^ static_cast<int>(b));                       \
    }                                                                                              \
    inline constexpr name operator~(name a) {                                                      \
        return static_cast<name>(~static_cast<int>(a));                                            \
    }
#define H1_ENUM_ID_BEGIN(name) enum name : int {
#define H1_ENUM_ID_END(name)                                                                       \
    }                                                                                              \
    ;
#define H1_ENUM_CONST_BEGIN(name) enum name : int {
#define H1_ENUM_CONST_END(name)                                                                    \
    }                                                                                              \
    ;
#define H1_ENUM_LOCAL(name, storage) name
#define H1_ENUM_SHARED(name, storage) H1EnumShared<name, storage>
#else
#define H1_STRICT_DOMAINS 0
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
#define H1_ENUM_ID_BEGIN(name) enum name {
#define H1_ENUM_ID_END(name)                                                                       \
    }                                                                                              \
    ;
#define H1_ENUM_CONST_BEGIN(name) enum name {
#define H1_ENUM_CONST_END(name)                                                                    \
    }                                                                                              \
    ;
#define H1_ENUM_LOCAL(name, storage) storage
#define H1_ENUM_SHARED(name, storage) storage
#define H1_ENUM_ARRAY(type, name, domain, count) type name[count]
#define H1_ENUM_ARRAY2(type, name, domain1, count1, domain2, count2) type name[count1][count2]
#define H1_ENUM_ARRAY_ROWS(type, name, domain, count, columns) type name[count][columns]
#define H1_ENUM_STEPPED(name)
#define H1_ENUM_BIT(domain, value) (1 << (value))
#define H1_ENUM_DECODE(domain, value) (value)
#define H1_ENUM_ENCODE(domain, value) (value)
#endif

#endif // HOMM1_DOMAINS_H
