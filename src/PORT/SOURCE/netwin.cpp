// The native counterpart of src/SOURCE/netwin.cpp, the NetBIOS transport.
// Network play is not available in the port yet (docs/port/README.md has the
// plan): initialization reports NetBIOS as missing, which the game turns
// into its own error message.

#include <H1/Ints.h>

#include <SOURCE/netwin.h>

u8 gNetbiosLana = 0;
u8 gNetbiosAvail = 0;

void nb_thr_ctl(void) {}

extern "C" u16 nb_init(u16) {
    return 1;
}

extern "C" void nb_term(i32) {}

extern "C" i16 nb_rcv(i32, u16, void*) {
    return 0;
}

extern "C" i16 nb_snd(i32, u16, u16, void*, i32) {
    return -1;
}

extern "C" i16 nb_sess(i32, i32, ...) {
    return -1;
}

extern "C" u8 nb_stat(i32, u16) {
    return NETBIOS_SESSION_ERROR;
}
