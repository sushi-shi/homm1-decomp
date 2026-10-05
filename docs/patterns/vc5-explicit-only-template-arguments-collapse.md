> Unmeasured hypothesis: observed with MSVC 5.0; not measured on this
> target's VC4 or VC6 compilers. Re-prove the mechanism before relying on it.

# Explicit-only function template arguments collapse

A function template whose parameter appears only as an explicit template
argument, never in the function's parameter types, is not distinguished per
argument by MSVC 5.0 SP3. Within one translation unit, every
`F<X>(...)` call can reach the same instantiation body.

Minimal probe (MSVC 5.0 SP3, `/O2 /MT /GX`):

```cpp
struct A {}; struct B {};
template<class T> struct Pool { static int s_table[4]; };
int Pool<A>::s_table[4];
int Pool<B>::s_table[4];
template<class T> inline int* Resolve(int i) { return &Pool<T>::s_table[i]; }
template<class T> struct PoolM { static int* Resolve(int i) { return &Pool<T>::s_table[i]; } };
void SetA(int i) { *Resolve<A>(i) = 1; }
void SetB(int i) { *Resolve<B>(i) = 2; }
void SetMA(int i) { *PoolM<A>::Resolve(i) = 3; }
void SetMB(int i) { *PoolM<B>::Resolve(i) = 4; }
```

`SetA` and `SetB` both relocate against `?s_table@?$Pool@UB@@@@2PAHA`; the
class-template static member keeps `SetMA` on `Pool<A>`.

Signature: masked bytes identical, one relocation target swapped for a sibling
instantiation's (a referent wall). It appears only in a TU
that uses two or more arguments of the same template; a single-use TU looks
correct.

This does not show which argument wins in general, or that the original source
avoided such templates. It shows that a source helper of this shape cannot
produce retail's distinct referents in a multi-use TU. Carry the type through a
parameter, a class template member, or the direct expression instead.
