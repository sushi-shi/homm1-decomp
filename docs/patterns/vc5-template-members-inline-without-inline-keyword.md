> Unmeasured hypothesis: observed with MSVC 5.0; not measured on this
> target's VC4 or VC6 compilers. Re-prove the mechanism before relying on it.

# Template members can expand without the inline keyword

Under MSVC 5.0 SP3 /O2 /Ob1, the following probe expands
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
call relocations.

This refutes a keyword-only eligibility rule. It does not show that every
template member expands, or identify a retail template argument.
Eligibility, definition visibility, and the decision at a particular call site
are separate questions. See [mixed expansion](inline-budget-emits-ool-comdat.md).
