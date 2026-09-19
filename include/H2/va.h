#ifndef HOMM2_VA_H
#define HOMM2_VA_H

#include <match.h>

#include <H2/Ints.h>

#define VAU(address)
#define SYMBOL(name)
#define SIZE(type, bytes) typedef char size_check_##type[(sizeof(type) == (bytes)) ? 1 : -1]
#define OVERRIDE

#endif
