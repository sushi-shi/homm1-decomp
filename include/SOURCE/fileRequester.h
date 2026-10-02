#ifndef HOMM1_SOURCE_FILEREQUESTER_H
#define HOMM1_SOURCE_FILEREQUESTER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 12 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;

// PickLoadGame allocates 0x1bc bytes; the constructor fills the packed span
// after baseManager and the list state at 0x1ae..0x1b4.
#pragma pack(push, 1)
class fileRequester : public baseManager {
public:
    char m_unknown30[0x18c];
    // --- constructors ---
    fileRequester(int, int, int, const char *, const char *, const char *);
    ~fileRequester();
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message &) OVERRIDE;
    // --- methods ---
    int InitializeFiles(char *, char *, int);
    int MapExistsForFilter(int);
    void SetupFiles(void);
    void CleanUpData(void);
    void SetOK(int);
    void DoKnob(void);
    void Update(int);
    char * GetFilename(void);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_FILEREQUESTER_H
