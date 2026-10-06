// The page of the browser build: keeps the player's game files in the
// browser's storage, then starts the game or the editor on them.
//
// The program's files live in an IndexedDB-backed folder (pre.js). This page
// copies a chosen folder into it once; the program then reads the game data
// from /homm1/game and writes its saved games, maps and settings back there.
// Nothing is uploaded anywhere.

'use strict';

(function () {
  const params = new URLSearchParams(location.search);
  const editor = params.get('program') === 'editor';
  const program = editor ? 'heroes-editor' : 'heroes';
  const GAME = '/homm1/game';
  const CD = '/homm1/cd';
  const FORGET_FLAG = 'homm1-forget';
  // IDBFS keeps a mount's files in a database named after the mount point.
  const DATABASE = '/homm1';

  const $ = (id) => document.getElementById(id);
  const logBox = $('log');

  function log(text) {
    logBox.textContent += text + '\n';
    console.log(text);
  }

  function setStatus(text) {
    $('status').textContent = text;
  }

  $(editor ? 'link-editor' : 'link-game').classList.add('current');
  document.title = editor ? 'Heroes of Might and Magic: scenario editor' : 'Heroes of Might and Magic';

  // ------------------------------------------------------------ audio

  // Browsers start audio suspended until the page is used. The program
  // opens its audio device later than the click that starts it, so every
  // audio context the page creates is resumed on the next click or key.
  const contexts = [];
  const NativeAudioContext = window.AudioContext || window.webkitAudioContext;
  if (NativeAudioContext) {
    const Tracked = class extends NativeAudioContext {
      constructor(...args) {
        super(...args);
        contexts.push(this);
      }
    };
    window.AudioContext = Tracked;
    window.webkitAudioContext = Tracked;
  }
  function unlockAudio() {
    for (const context of contexts) {
      if (context.state === 'suspended')
        context.resume();
    }
  }
  for (const type of ['pointerdown', 'keydown', 'touchend'])
    window.addEventListener(type, unlockAudio, true);

  // ------------------------------------------------------------ files

  // Case-insensitive lookups and creation inside the program's file system,
  // so that files copied in again under another spelling replace the
  // stored ones instead of sitting beside them.
  function entryIn(folder, name) {
    let entries;
    try {
      entries = Module.FS.readdir(folder);
    } catch (e) {
      return null;
    }
    if (entries.includes(name))
      return name;
    const lower = name.toLowerCase();
    return entries.filter((entry) => entry.toLowerCase() === lower).sort()[0] || null;
  }

  function resolve(path) {
    let current = '';
    for (const part of path.split('/').filter(Boolean)) {
      const found = entryIn(current || '/', part);
      if (found === null)
        return null;
      current += '/' + found;
    }
    return current;
  }

  function folderFor(path) {
    let current = '';
    for (const part of path.split('/').filter(Boolean)) {
      const found = entryIn(current || '/', part);
      current += '/' + (found === null ? part : found);
      if (found === null)
        Module.FS.mkdir(current);
    }
    return current;
  }

  function writeFile(path, bytes) {
    const slash = path.lastIndexOf('/');
    const folder = folderFor(path.slice(0, slash));
    const name = path.slice(slash + 1);
    const existing = entryIn(folder, name);
    Module.FS.writeFile(folder + '/' + (existing === null ? name : existing), bytes);
  }

  function hasGame() {
    return resolve(GAME + '/DATA/HEROES.AGG') !== null;
  }

  // Where each chosen file goes: files under the folder that holds
  // DATA/HEROES.AGG into the game folder, CD music tracks into the CD
  // folder, a help file found elsewhere into the game's HELP folder.
  function plan(files) {
    const pathOf = (file) => (file.webkitRelativePath || file.name).replace(/\\/g, '/');
    let root = null;
    for (const file of files) {
      const path = pathOf(file);
      const lower = path.toLowerCase();
      const at = lower.length - 'data/heroes.agg'.length;
      if (lower.endsWith('data/heroes.agg') && (at === 0 || lower[at - 1] === '/')) {
        if (root === null || at < root.length)
          root = path.slice(0, at);
      }
    }
    const copies = [];
    for (const file of files) {
      const path = pathOf(file);
      const name = path.slice(path.lastIndexOf('/') + 1);
      if (root !== null && path.startsWith(root)) {
        // The Windows program files are of no use here.
        if (!/\.(exe|dll)$/i.test(name))
          copies.push({ file, to: GAME + '/' + path.slice(root.length) });
      } else if (/(^|\/)tracks\/[^/]+\.ogg$/i.test(path) || /^\d\d-audiotrack \d\d\.ogg$/i.test(name)) {
        copies.push({ file, to: CD + '/Tracks/' + name });
      } else if (/^track \d\d\.flac$/i.test(name)) {
        // The edition's lossless music (LosslessAudio) lives in the game folder.
        copies.push({ file, to: GAME + '/Audio/' + name });
      } else if (/^heroes\.(hlp|cnt)$/i.test(name)) {
        copies.push({ file, to: GAME + '/HELP/' + name });
      }
    }
    return { root, copies };
  }

  async function importFiles(fileList) {
    const files = Array.from(fileList);
    if (files.length === 0)
      return;
    const { root, copies } = plan(files);
    if (root === null && !hasGame()) {
      setStatus('The chosen files do not hold the game: no DATA/HEROES.AGG was found among them.');
      return;
    }
    if (copies.length === 0) {
      setStatus('None of the chosen files belong to the game.');
      return;
    }
    if (navigator.storage && navigator.storage.persist)
      navigator.storage.persist().catch(() => {});
    const progress = $('progress');
    progress.hidden = false;
    const total = copies.reduce((sum, copy) => sum + copy.file.size, 0);
    let done = 0;
    try {
      for (const copy of copies) {
        progress.textContent = `Copying ${copy.to.slice('/homm1/'.length)} (${Math.round((100 * done) / Math.max(total, 1))}%)`;
        writeFile(copy.to, new Uint8Array(await copy.file.arrayBuffer()));
        done += copy.file.size;
      }
      progress.textContent = 'Storing the files in the browser...';
      await Module.homm1Persist();
      progress.textContent = `Stored ${copies.length} files (${(total / 1048576).toFixed(1)} MB).`;
    } catch (error) {
      progress.textContent = 'Copying failed: ' + error;
      log(String(error && error.stack || error));
    }
    showState();
  }

  function listSaves() {
    const list = $('saves');
    list.textContent = '';
    for (const folder of ['GAMES', 'MAPS']) {
      const path = resolve(GAME + '/' + folder);
      if (path === null)
        continue;
      for (const name of Module.FS.readdir(path).sort()) {
        if (name === '.' || name === '..' || !/\.(gm\d|map|cmp)$/i.test(name))
          continue;
        const full = path + '/' + name;
        const item = document.createElement('li');
        const link = document.createElement('a');
        link.textContent = folder + '/' + name;
        link.href = '#';
        link.addEventListener('click', (event) => {
          event.preventDefault();
          const blob = new Blob([Module.FS.readFile(full)], { type: 'application/octet-stream' });
          const url = URL.createObjectURL(blob);
          const anchor = document.createElement('a');
          anchor.href = url;
          anchor.download = name;
          anchor.click();
          setTimeout(() => URL.revokeObjectURL(url), 10000);
        });
        item.appendChild(link);
        list.appendChild(item);
      }
    }
  }

  function showState() {
    $('setup').hidden = false;
    if (hasGame()) {
      const music = resolve(CD + '/Tracks') !== null ? ' with the CD music' : '';
      const help = resolve(GAME + '/HELP/HEROES.HLP') !== null ? ' and the help file' : '';
      setStatus(`The game files are stored in this browser${music}${help}.`);
      $('ready').hidden = false;
      listSaves();
    } else {
      setStatus('No game files are stored in this browser yet.');
      $('ready').hidden = true;
    }
  }

  // ------------------------------------------------------------ program

  // Opens a document the program wrote (the converted help) in a new tab.
  // A blocked pop-up leaves a link on the page instead.
  function openDocument(bytes, type) {
    const url = URL.createObjectURL(new Blob([bytes], { type }));
    const opened = window.open(url, '_blank');
    if (!opened) {
      const popup = $('popup');
      popup.hidden = false;
      popup.textContent = 'The browser blocked a new tab. ';
      const link = document.createElement('a');
      link.href = url;
      link.target = '_blank';
      link.textContent = 'Open the help';
      popup.appendChild(link);
    }
  }

  // The canvas holds the game's image at its own size (SDL sets it, menu
  // bar included); the page shows it as large as the window allows, keeping
  // its proportions. SDL maps pointer positions through the shown size.
  //
  // SDL measures the canvas's CSS size when it creates its window and, when
  // something already sizes it, takes that as the window size; so the page
  // sizes the canvas only once SDL has set its pixel size.
  let fitting = false;
  function fitCanvas() {
    const canvas = $('canvas');
    if (!fitting || canvas.width <= 1 || canvas.height <= 1)
      return;
    const room = $('screen').clientWidth;
    const height = window.innerHeight - canvas.getBoundingClientRect().top - 16;
    const scale = Math.max(0.25, Math.min(room / canvas.width, height / canvas.height));
    canvas.style.setProperty('width', Math.floor(canvas.width * scale) + 'px', 'important');
    canvas.style.setProperty('height', Math.floor(canvas.height * scale) + 'px', 'important');
  }
  window.addEventListener('resize', fitCanvas);
  new MutationObserver(fitCanvas).observe($('canvas'), { attributes: true, attributeFilter: ['width', 'height'] });

  function start() {
    $('setup').hidden = true;
    $('screen').hidden = false;
    // SDL opens its audio when the program asks for it, after this click;
    // some browsers (Firefox) only let a context start during the click
    // itself. SDL takes the context it finds here instead of creating one.
    if (NativeAudioContext) {
      Module.SDL3 = Module.SDL3 || {};
      if (!Module.SDL3.audioContext)
        Module.SDL3.audioContext = new window.AudioContext();
    }
    unlockAudio();
    const args = ['--data', GAME];
    const extra = (params.get('args') || '').split(/\s+/).filter(Boolean);
    $('canvas').focus();
    fitting = true;
    Module.callMain(args.concat(extra));
  }

  async function forget() {
    if (!confirm('Remove the stored game files, saved games and maps from this browser?'))
      return;
    localStorage.setItem(FORGET_FLAG, '1');
    location.reload();
  }

  const environment = {};
  if (params.get('quiet'))
    environment.HOMM1_NO_DIALOGS = '1';

  window.Module = {
    canvas: $('canvas'),
    homm1Environment: environment,
    homm1OpenDocument: openDocument,
    print: log,
    printErr: log,
    onRuntimeInitialized: showState,
    onExit: (code) => {
      $('ended').hidden = false;
      $('ended').textContent = `The program ended (${code}). Reload the page to start again.`;
    },
    onAbort: (what) => {
      $('ended').hidden = false;
      $('ended').textContent = 'The program stopped with an error: ' + what;
    },
  };

  // One copy at a time, in the order the files were chosen.
  let copying = Promise.resolve();
  function queueImport(event) {
    const files = Array.from(event.target.files);
    event.target.value = '';
    copying = copying.then(() => importFiles(files));
  }
  $('pick-folder').addEventListener('change', queueImport);
  $('pick-files').addEventListener('change', queueImport);
  $('play').addEventListener('click', start);
  $('forget').addEventListener('click', forget);
  $('play').textContent = editor ? 'Open the editor' : 'Play';

  function load() {
    const script = document.createElement('script');
    script.src = program + '.js';
    script.onerror = () => setStatus(`Cannot load ${program}.js.`);
    document.body.appendChild(script);
    $('setup').hidden = false;
  }

  // Removing the stored files has to happen before the program opens the
  // database, hence through a reload.
  if (localStorage.getItem(FORGET_FLAG)) {
    localStorage.removeItem(FORGET_FLAG);
    const request = indexedDB.deleteDatabase(DATABASE);
    request.onsuccess = request.onerror = request.onblocked = load;
  } else {
    load();
  }
})();
