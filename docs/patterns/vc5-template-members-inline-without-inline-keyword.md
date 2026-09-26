> Imported from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
> This is donor evidence, not a validated HoMM1 VC4 rule. Commands and source
> examples describe that donor. Re-prove applicable mechanisms with VC4.

# Template members can expand without the inline keyword

Under the pinned compiler's /O2 /Ob1, the following recorded probe expands
`Array<int>::At`, while the ordinary out-of-class `Plain::At` remains a call.
With /Ob0, both callers retain calls.

```cpp
template<class T> struct Array {
    T* data;
    T& At(int i);
};
template<class T> T& Array<T>::At(int i) { return data[i]; }
struct Plain {
    int* data;
    int& At(int i);
};
int& Plain::At(int i) { return data[i]; }
int UseTemplate(Array<int>* a, int i) { return a->At(i); }
int UsePlain(Plain* a, int i) { return a->At(i); }
```

Compile both settings with MSVC 5.0 SP3 /O2 /MT and inspect instructions and
call relocations. The [original probe](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/vc5-template-members-inline-without-inline-keyword.md)
records the indexed-load bytes and call offsets.

This refutes a keyword-only eligibility rule. It does not show that every
template member expands, or identify a retail template argument.
Eligibility, definition visibility, and the decision at a particular call site
are separate questions. See [mixed expansion](inline-budget-emits-ool-comdat.md).
