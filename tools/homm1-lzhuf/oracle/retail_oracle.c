/*
 * Retail LZHUF oracle: runs EncodeData/DecodeData from the pinned HEROES.EXE.
 *
 *   image_stub IMAGE SCRIPT OUTDIR ENCODE DECODE POLL WINDOW WINDOW_SIZE
 *
 * The addresses are virtual addresses supplied by `homm1 verify lzhuf-oracle`
 * from the reconstruction's claims. The image is linked /FIXED without
 * relocations, so its sections must occupy their preferred base. Under Wine
 * that range is taken by early mappings unless the main executable claims
 * it, so `image_stub.c` is a /FIXED executable at the same base whose
 * uninitialised data spans the image; it only calls `oracle_main` in this
 * DLL, which overwrites the stub with the retail sections and never returns.
 *
 * Nothing is resolved or initialised beyond the section contents: the codec
 * calls only the statically linked CRT memory routines and PollSound, whose
 * first byte is replaced by RET. The script language matches
 * `homm1-lzhuf replay`:
 *
 *   reset                  reload the image (fresh process state)
 *   window INPUT           copy INPUT over the start of text_buf
 *   encode INPUT OUTPUT    EncodeData
 *   decode INPUT OUTPUT    DecodeData
 *   dump-window OUTPUT     write text_buf
 *
 * Inputs are relative to the script's directory, outputs to OUTDIR.
 * Compiled with the pinned VC6 toolchain under Wine.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (__cdecl *EncodeDataFn)(char *destination, char *source, unsigned length);
typedef int (__cdecl *DecodeDataFn)(char *destination, char *source);

/* Zero bytes after each stream: the game's decoder may read ahead. */
#define READ_AHEAD_PAD 64

static unsigned char *file_bytes;
static long file_size;
static IMAGE_NT_HEADERS *nt;
static unsigned char *image;
static DWORD encode_va, decode_va, poll_va, window_va, window_size;
static char script_dir[MAX_PATH];
static char output_dir[MAX_PATH];

static void fail(const char *what, const char *detail)
{
    fprintf(stderr, "retail_oracle: %s%s%s\n", what, detail ? ": " : "", detail ? detail : "");
    exit(1);
}

static unsigned char *read_file(const char *path, long *size)
{
    FILE *f = fopen(path, "rb");
    unsigned char *bytes;
    if (!f)
        fail("cannot open", path);
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    bytes = (unsigned char *)malloc(*size + READ_AHEAD_PAD + 1);
    if (!bytes)
        fail("out of memory reading", path);
    if (*size && fread(bytes, 1, *size, f) != (size_t)*size)
        fail("cannot read", path);
    memset(bytes + *size, 0, READ_AHEAD_PAD + 1);
    fclose(f);
    return bytes;
}

static void write_file(const char *path, const void *bytes, long size)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        fail("cannot create", path);
    if (size && fwrite(bytes, 1, size, f) != (size_t)size)
        fail("cannot write", path);
    fclose(f);
}

static void join(char *out, const char *dir, const char *name)
{
    if (strlen(dir) + strlen(name) + 2 > MAX_PATH)
        fail("path too long", name);
    sprintf(out, "%s\\%s", dir, name);
}

static unsigned char *at(DWORD va, DWORD size)
{
    DWORD base = nt->OptionalHeader.ImageBase;
    if (va < base || va - base + size > nt->OptionalHeader.SizeOfImage)
        fail("address outside the image", NULL);
    return image + (va - base);
}

/* Map the image once, then (re)load headers and sections on every reset. */
static void reset_image(void)
{
    IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION(nt);
    WORD index;
    if (!image) {
        DWORD old;
        MEMORY_BASIC_INFORMATION stub;
        image = (unsigned char *)nt->OptionalHeader.ImageBase;
        if (!VirtualQuery(image, &stub, sizeof stub)
            || stub.AllocationBase != (void *)GetModuleHandle(NULL)
            || !VirtualQuery(image + nt->OptionalHeader.SizeOfImage - 1, &stub, sizeof stub)
            || stub.AllocationBase != (void *)GetModuleHandle(NULL))
            fail("the stub executable does not span the image's address range", NULL);
        if (!VirtualProtect(image, nt->OptionalHeader.SizeOfImage, PAGE_EXECUTE_READWRITE, &old))
            fail("cannot make the image range writable", NULL);
    }
    memset(image, 0, nt->OptionalHeader.SizeOfImage);
    memcpy(image, file_bytes, nt->OptionalHeader.SizeOfHeaders);
    for (index = 0; index < nt->FileHeader.NumberOfSections; ++index, ++section) {
        DWORD raw = section->SizeOfRawData;
        if (section->Misc.VirtualSize && raw > section->Misc.VirtualSize)
            raw = section->Misc.VirtualSize;
        if (section->PointerToRawData + raw > (DWORD)file_size)
            fail("section outside the file", (const char *)section->Name);
        memcpy(image + section->VirtualAddress, file_bytes + section->PointerToRawData, raw);
    }
    *at(poll_va, 1) = 0xC3; /* PollSound: ret */
}

static void encode(const char *input, const char *output)
{
    char path[MAX_PATH];
    long size;
    unsigned char *source;
    char *destination;
    long capacity;
    int code;

    join(path, script_dir, input);
    source = read_file(path, &size);
    /* At most two code bytes per window position; empty input still codes
       65536 positions. */
    capacity = 4 * (size + 65536) + 4096;
    destination = (char *)malloc(capacity);
    if (!destination)
        fail("out of memory encoding", input);
    code = ((EncodeDataFn)encode_va)(destination, (char *)source, (unsigned)size);
    if (code < 0 || code + 4 > capacity)
        fail("EncodeData returned an impossible length for", input);
    join(path, output_dir, output);
    write_file(path, destination, code + 4);
    free(destination);
    free(source);
}

static void decode(const char *input, const char *output)
{
    char path[MAX_PATH];
    long size;
    unsigned char *stream;
    unsigned long declared;
    char *destination;
    int decoded;

    join(path, script_dir, input);
    stream = read_file(path, &size);
    if (size < 4)
        fail("stream without a length prefix", input);
    declared = ((unsigned long)stream[0] << 24) | ((unsigned long)stream[1] << 16)
             | ((unsigned long)stream[2] << 8) | stream[3];
    destination = (char *)malloc(declared + 1);
    if (!destination)
        fail("out of memory decoding", input);
    decoded = ((DecodeDataFn)decode_va)(destination, (char *)stream);
    if ((unsigned long)decoded != declared)
        fail("DecodeData returned a different length for", input);
    join(path, output_dir, output);
    write_file(path, destination, decoded);
    free(destination);
    free(stream);
}

static void load_window(const char *input)
{
    char path[MAX_PATH];
    long size;
    unsigned char *bytes;
    join(path, script_dir, input);
    bytes = read_file(path, &size);
    if ((DWORD)size > window_size)
        fail("window file larger than text_buf", input);
    memcpy(at(window_va, window_size), bytes, size);
    free(bytes);
}

static void dump_window(const char *output)
{
    char path[MAX_PATH];
    join(path, output_dir, output);
    write_file(path, at(window_va, window_size), window_size);
}

static DWORD number(const char *text)
{
    return strtoul(text, NULL, 0);
}

/* Called by image_stub, whose code is overwritten here: always exits. */
__declspec(dllexport) void __cdecl oracle_main(int argc, char **argv)
{
    FILE *script;
    char line[2 * MAX_PATH + 64];
    char *slash;
    int line_number = 0;

    if (argc != 9) {
        fprintf(stderr, "usage: image_stub IMAGE SCRIPT OUTDIR ENCODE DECODE POLL WINDOW WINDOW_SIZE\n");
        exit(2);
    }
    file_bytes = read_file(argv[1], &file_size);
    if (file_size < (long)sizeof(IMAGE_DOS_HEADER)
        || ((IMAGE_DOS_HEADER *)file_bytes)->e_magic != IMAGE_DOS_SIGNATURE)
        fail("not an MZ image", argv[1]);
    nt = (IMAGE_NT_HEADERS *)(file_bytes + ((IMAGE_DOS_HEADER *)file_bytes)->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        fail("not a PE image", argv[1]);

    strcpy(script_dir, argv[2]);
    slash = strrchr(script_dir, '\\');
    if (!slash)
        slash = strrchr(script_dir, '/');
    if (slash)
        *slash = '\0';
    else
        strcpy(script_dir, ".");
    strcpy(output_dir, argv[3]);
    encode_va = number(argv[4]);
    decode_va = number(argv[5]);
    poll_va = number(argv[6]);
    window_va = number(argv[7]);
    window_size = number(argv[8]);
    reset_image();
    at(encode_va, 1);
    at(decode_va, 1);

    script = fopen(argv[2], "r");
    if (!script)
        fail("cannot open", argv[2]);
    while (fgets(line, sizeof line, script)) {
        char *op, *first, *second, *comment;
        ++line_number;
        comment = strchr(line, '#');
        if (comment)
            *comment = '\0';
        op = strtok(line, " \t\r\n");
        if (!op)
            continue;
        first = strtok(NULL, " \t\r\n");
        second = strtok(NULL, " \t\r\n");
        if (!strcmp(op, "reset") && !first)
            reset_image();
        else if (!strcmp(op, "window") && first && !second)
            load_window(first);
        else if (!strcmp(op, "encode") && second)
            encode(first, second);
        else if (!strcmp(op, "decode") && second)
            decode(first, second);
        else if (!strcmp(op, "dump-window") && first && !second)
            dump_window(first);
        else {
            fprintf(stderr, "retail_oracle: %s:%d: unknown operation\n", argv[2], line_number);
            exit(1);
        }
    }
    fclose(script);
    fflush(stdout);
    exit(0);
}
