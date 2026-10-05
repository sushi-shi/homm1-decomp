/*
 * Address-space stub for retail_oracle.dll: a /FIXED executable at
 * HEROES.EXE's base whose uninitialised data covers the retail image, so Wine
 * maps nothing else there. oracle_main replaces this image and exits.
 */
#define RETAIL_IMAGE_SPAN 0xE0000 /* >= HEROES.EXE SizeOfImage (0xDC000) */

__declspec(dllimport) void __cdecl oracle_main(int argc, char **argv);

static char span[RETAIL_IMAGE_SPAN];

int main(int argc, char **argv)
{
    span[0] = 0;
    oracle_main(argc, argv);
    return 1;
}
