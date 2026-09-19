#include "Domains.h"
#include <stddef.h>

H1_ENUM_BEGIN(ProbeDomain)
    PROBE_FIRST = 1,
    PROBE_SECOND = 2
H1_ENUM_END(ProbeDomain)

struct ProbeLayout {
    char first;
    H1_ENUM_STORAGE(ProbeDomain, short) domain;
    int last;
};
typedef char AssertLayout[sizeof(ProbeLayout) == 8 ? 1 : -1];
typedef char AssertOffset[offsetof(ProbeLayout, last) == 4 ? 1 : -1];

class Probe {
public:
    Probe();
    ~Probe();
    virtual int value(int input);
    static int twice(int input);
    int overload(int input);
    int overload(short input);
    int field;
};

Probe::Probe() : field(1) {}
Probe::~Probe() { field = 0; }
int Probe::value(int input) { return field + input; }
int Probe::twice(int input) { return input * 2; }
int Probe::overload(int input) { return field + input; }
int Probe::overload(short input) { return field - input; }

extern "C" int __stdcall probe_stdcall(int a, int b) { return a + b; }
extern "C" int __cdecl probe_cdecl(int a) { return a + 1; }
static int probe_static(int a) { return a - 1; }
int probe_switch(int a) {
    switch (a) {
    case 0: return probe_static(a);
    case 1: return 7;
    case 2: return 11;
    case 3: return 19;
    case 4: return 29;
    default: return 0;
    }
}
extern void probe_throwing_call();
int probe_unwind() {
    Probe local;
    probe_throwing_call();
    return local.field;
}
