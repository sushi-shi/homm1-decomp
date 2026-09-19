#ifndef HOMM1_SOURCE_HIGHSCOREMANAGER_H
#define HOMM1_SOURCE_HIGHSCOREMANAGER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 5 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;

class highScoreManager {
public:
    // --- constructors ---
    highScoreManager(void);
    // --- virtual methods (vtable order) ---
    virtual int Open(int) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual int Main(struct tag_message &) OVERRIDE;
    // --- methods ---
    void Update(void);
};
#endif // HOMM1_SOURCE_HIGHSCOREMANAGER_H
