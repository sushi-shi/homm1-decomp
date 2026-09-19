#include "Domains.h"
#include "match.h"

H1_ENUM_BEGIN(ProbeDomain)
    PROBE_FIRST = 1,
    PROBE_SECOND = 2
H1_ENUM_END(ProbeDomain)

struct ProbeLayout {
    char first;
    H1_ENUM_STORAGE(ProbeDomain, short) domain;
    int last;
};

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

// Synthetic identities for binding tests; these are NOT game claims.
VA(0x00401000, 1) Probe::Probe() : field(1) {}
VA(0x00401010, 1) Probe::~Probe() { field = 0; }
VA(0x00401020, 1) int Probe::value(int input) { return field + input; }
VA(0x00401030, 1) int Probe::twice(int input) { return input * 2; }
VA(0x00401040, 1) int Probe::overload(int input) { return field + input; }
VA(0x00401050, 1) int Probe::overload(short input) { return field - input; }
VA(0x00401060, 1) int probe_dispatch(Probe *object) { return object->value(3); }

extern "C" VA(0x00401070, 1) int __stdcall probe_stdcall(int a, int b) { return a + b; }
extern "C" VA(0x00401080, 1) int __cdecl probe_cdecl(int a) { return a + 1; }
VA(0x00401090, 1) static int probe_static(int a) { return a - 1; }
VA(0x004010a0, 1) int probe_switch(int a) {
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
VA(0x004010b0, 1) int probe_unwind() {
    Probe local;
    probe_throwing_call();
    return local.field;
}
