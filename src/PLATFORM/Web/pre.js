// Linked ahead of the browser build (--pre-js): the program's file system.
//
// Everything the program reads or writes lives under /homm1, an IndexedDB
// backed folder of the page's origin:
//   /homm1/game     the player's game folder (DATA, MAPS, GAMES, SOUND, ANIM,
//                   HELP), copied there once by the page (index.html);
//   /homm1/cd       the CD's music tracks (TRACKS), when the player gave them;
//   /homm1/config   the settings file and the converted help.
// It is loaded into memory before the program starts and written back to
// IndexedDB shortly after every file the program writes is closed (saved
// games, the editor's maps, the settings), so saves outlive the page.

Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
  FS.mkdir('/homm1');
  FS.mount(IDBFS, { autoPersist: true }, '/homm1');
  addRunDependency('homm1-storage');
  FS.syncfs(true, function (error) {
    if (error)
      err('cannot load the stored game files: ' + error);
    for (const folder of ['/homm1/game', '/homm1/cd', '/homm1/config']) {
      try {
        FS.mkdir(folder);
      } catch (e) {
        // already there
      }
    }
    removeRunDependency('homm1-storage');
  });
  ENV['XDG_CONFIG_HOME'] = '/homm1/config';
  ENV['HOMM1_CD'] = '/homm1/cd';
  const extra = Module['homm1Environment'] || {};
  for (const name of Object.keys(extra))
    ENV[name] = extra[name];
});

// Waits until every pending write has reached IndexedDB.
Module['homm1Persist'] = function () {
  return new Promise(function (resolve, reject) {
    FS.syncfs(false, function (error) {
      if (error)
        reject(error);
      else
        resolve();
    });
  });
};
