"""Portable static localization catalog; shared with generated source builds.

    locales/messages.pot   the message IDs, generated from the source's
                           localization::Tr/Chars calls (`update` below)
    locales/<lang>.po      one translation per language (msgid = the ID)
    locales/<lang>.json    the language's descriptor: Windows code page,
                           resource language, system locale, glyph set and
                           keyboard table

Every language is a translation; none is the source. IDs named `locale.*` are
not messages: the descriptor supplies them (`locale.keyboard`).

    python3 catalog.py check            validate every language
    python3 catalog.py update [--check] regenerate the template and rewrite
                                        each .po in template order (a
                                        descriptor without a .po gets one)
"""
from __future__ import annotations

import ast
import codecs
import hashlib
import json
import re
from dataclasses import dataclass, field
from pathlib import Path

# One lexer shared by the renderer and the no-hidden-text gate. Comments, quoted
# backslashes and character literals must not be mistaken for localization calls.
TOKEN = re.compile(
    r'(?P<space>\s+)|(?P<comment>//[^\n]*|/\*.*?\*/)|'
    r'(?P<string>(?:u8|u|U|L)?"(?:\\.|[^"\\])*")|'
    r"(?P<char>(?:u|U|L)?'(?:\\.|[^'\\])*')|"
    r'(?P<identifier>[A-Za-z_]\w*)|(?P<punct>::|.)', re.S)
ID = re.compile(r'[a-z][a-z0-9_]*(?:\.[A-Za-z0-9_]+)+\Z')
NUMERIC_ESCAPE = re.compile(r'\\(?:x[0-9a-fA-F]+|[0-7]{1,3}|u[0-9a-fA-F]{4}|U[0-9a-fA-F]{8})')

LOCALES = 'locales'
TEMPLATE = 'messages.pot'
#: Authored files that may hold localization calls.
SOURCE_DIRS = ('src', 'include')
SOURCE_SUFFIXES = ('.cpp', '.h', '.c', '.hpp', '.inc', '.rc')
#: IDs a locale descriptor supplies instead of its .po.
DESCRIPTOR_PREFIX = 'locale.'
KEYBOARD_ID = 'locale.keyboard'
#: Characters the game's text renderer draws besides printable ASCII, by the
#: descriptor's "glyphs" name. FONT.cpp maps Windows-1251 Cyrillic to the
#: glyph order of Buka's AGG fonts, and guillemets, the em dash and the
#: numero sign to the Tournament Edition's extra glyphs (ASCII look-alikes
#: with fonts that lack them); every other byte above 0x7F draws blank.
GLYPHS = {
    'ascii': '',
    'cyrillic': ''.join(chr(c) for c in range(0x410, 0x450)) + 'Ёё«»—№',
}
FIXED_WIDTH_NOTE = 'fixed-width character field'
RESOURCE_LANGUAGE = 'HOMM1_RESOURCE_LANGUAGE'
RESOURCE_SUBLANGUAGE = 'HOMM1_RESOURCE_SUBLANGUAGE'


def tokens(text):
    return [m for m in TOKEN.finditer(text) if m.lastgroup not in ('space', 'comment')]


def quoted(value):
    return json.dumps(value, ensure_ascii=False)


def encode(value: str, codepage: int = 1251) -> bytes:
    """Exact Windows code page bytes; unrepresentable text is an error."""
    return value.encode(f'cp{codepage}')


def literal(value: str, codepage: int = 1251) -> str:
    """ASCII C literal encoding exact code page bytes (fixed-width octal)."""
    out = []
    for byte in encode(value, codepage):
        if byte in (34, 92):
            out.append('\\' + chr(byte))
        elif 32 <= byte < 127:
            out.append(chr(byte))
        else:
            out.append('\\%03o' % byte)
    return '"' + ''.join(out) + '"'


def character_initializer(value: str, codepage: int = 1251) -> str:
    """A fixed-width char-array initializer, without an implicit terminator."""
    result = []
    for byte in encode(value, codepage):
        if byte == 39:
            escaped = "\\'"
        elif byte == 92:
            escaped = "\\\\"
        elif 32 <= byte < 127:
            escaped = chr(byte)
        else:
            escaped = '\\%03o' % byte
        result.append("'" + escaped + "'")
    if not result:
        raise ValueError('character initializer cannot be empty')
    return '{' + ', '.join(result) + '}'


def resource_literal(value: str) -> str:
    """RC wide literal: numeric narrow escapes are not code-page decoded."""
    data = value.encode('utf-16le')
    return 'L"' + ''.join('\\x%04x' % int.from_bytes(data[i:i + 2], 'little')
                           for i in range(0, len(data), 2)) + '"'


# --------------------------------------------------------------------------
# Source calls
# --------------------------------------------------------------------------

def find_calls(text):
    """(start, end, id, 'Tr'|'Chars') for every localization call in `text`."""
    ts = tokens(text)
    for i, token in enumerate(ts):
        if token.group() != 'localization':
            continue
        tail = ts[i:i + 6]
        if (len(tail) < 3 or tail[1].group() != '::'
                or tail[2].group() not in ('Tr', 'Chars')):
            continue
        if (len(tail) != 6 or tail[3].group() != '(' or
                tail[4].lastgroup != 'string' or tail[5].group() != ')'):
            raise ValueError('localization::' + tail[2].group() + ' requires one literal semantic ID')
        key = ast.literal_eval(tail[4].group())
        if not ID.fullmatch(key):
            raise ValueError(f'invalid message ID: {key}')
        yield token.start(), tail[5].end(), key, tail[2].group()


def source_files(root):
    root = Path(root)
    for directory in SOURCE_DIRS:
        for path in sorted((root / directory).rglob('*')):
            if path.is_file() and path.suffix in SOURCE_SUFFIXES:
                yield path


def hidden_text_errors(text, *, allow_unicode=False):
    """Reject numeric escapes; spell controls as \n, \t, etc. (NUL as \0).

    Escaped backslashes are consumed as a pair, so a path containing the text
    ``\\\\x`` is not mistaken for a byte escape. Checks strings AND characters.
    Non-ASCII text belongs in the catalog, not in inline game literals.
    """
    errors = []
    for token in tokens(text):
        if token.lastgroup not in ('string', 'char'):
            continue
        raw = token.group()
        for escape in re.finditer(r'\\(?:x[0-9a-fA-F]+|[0-7]{1,3}|u[0-9a-fA-F]{4}|U[0-9a-fA-F]{8}|.)', raw, re.S):
            value = escape.group()
            if not NUMERIC_ESCAPE.fullmatch(value):
                continue
            if value != '\\0':
                errors.append((text.count('\n', 0, token.start()) + 1, 'numeric text escape ' + value))
        if not allow_unicode and any(ord(c) >= 128 for c in raw):
            errors.append((text.count('\n', 0, token.start()) + 1, 'inline non-ASCII text; use a catalog ID'))
    return errors


# --------------------------------------------------------------------------
# PO files
# --------------------------------------------------------------------------

@dataclass
class Entry:
    msgid: str
    msgstr: str = ''
    references: list = field(default_factory=list)   # '#:' file names
    flags: list = field(default_factory=list)        # '#,' flags
    notes: list = field(default_factory=list)        # '#.' extracted comments
    comments: list = field(default_factory=list)     # '# ' translator comments


def parse_po(text):
    """(header fields, [Entry]) of a PO file in the subset this catalog writes:
    msgid/msgstr pairs separated by blank lines, comments before each msgid."""
    entries, header = [], {}
    pending, current, active, has_msgstr = Entry(''), None, None, False
    for number, line in enumerate([*text.splitlines(), ''], 1):
        line = line.strip()
        if not line:
            if current is not None:
                if not has_msgstr:
                    raise ValueError(f'{current.msgid}: msgid without msgstr')
                entries.append(current)
            pending, current, active, has_msgstr = Entry(''), None, None, False
        elif line.startswith('#'):
            if current is not None:
                raise ValueError(f'line {number}: comment inside an entry')
            kind, rest = line[1:2], line[2:].strip()
            if kind == ',':
                pending.flags += [flag.strip() for flag in rest.split(',') if flag.strip()]
            elif kind == ':':
                pending.references += rest.split()
            elif kind == '.':
                pending.notes.append(rest)
            elif kind in ('~', '|'):
                raise ValueError(f'line {number}: obsolete and previous-text entries are not kept')
            else:
                pending.comments.append(line[1:].strip())
        elif line.startswith('"'):
            if active is None:
                raise ValueError(f'line {number}: continuation without a field')
            setattr(current, active, getattr(current, active) + ast.literal_eval(line))
        else:
            match = re.fullmatch(r'(msgid|msgstr)\s+(".*")', line)
            if match is None:
                raise ValueError(f'line {number}: unsupported PO field: {line}')
            value = ast.literal_eval(match[2])
            if match[1] == 'msgid':
                if current is not None:
                    raise ValueError(f'line {number}: msgid without a preceding blank line')
                current, pending = pending, Entry('')
                current.msgid = value
            else:
                if current is None or has_msgstr:
                    raise ValueError(f'line {number}: msgstr without its msgid')
                current.msgstr, has_msgstr = value, True
            active = match[1]
    if entries and entries[0].msgid == '':
        for line in entries.pop(0).msgstr.splitlines():
            name, _, value = line.partition(':')
            header[name.strip()] = value.strip()
    return header, entries


def write_po(header_fields, preamble, entries):
    """The canonical text of a PO file."""
    out = [f'# {line}' if line else '#' for line in preamble]
    out += ['msgid ""', 'msgstr ""']
    out += [quoted(f'{name}: {value}\n') for name, value in header_fields]
    for entry in entries:
        out.append('')
        out += [f'# {comment}' if comment else '#' for comment in entry.comments]
        out += [f'#. {note}' for note in entry.notes]
        if entry.references:
            out.append('#: ' + ' '.join(entry.references))
        if entry.flags:
            out.append('#, ' + ', '.join(entry.flags))
        out.append(f'msgid {quoted(entry.msgid)}')
        out.append(f'msgstr {quoted(entry.msgstr)}')
    return '\n'.join(out) + '\n'


# Compare conversion types, width/precision star arguments, and length modifiers.
# Clang checks the actual call-site C++ argument types after expansion.
FORMAT = re.compile(r'(?<![0-9])%(?:[-+ #0]*)(?:\*|\d+)?(?:\.(?:\*|\d+))?(?:hh|ll|I64|[hlLjzt])?[diouxXfFeEgGaAcspn%]')


def format_signature(text):
    result = []
    for match in FORMAT.finditer(text):
        spec = match.group()
        if spec == '%%':
            continue
        result.extend('*' for _ in range(spec.count('*')))
        kind = re.search(r'(hh|ll|I64|[hlLjzt])?([diouxXfFeEgGaAcspn%])$', spec)
        result.append((kind[1] or '') + kind[2])
    return result


# --------------------------------------------------------------------------
# Languages
# --------------------------------------------------------------------------

@dataclass(frozen=True)
class Locale:
    code: str
    name: str
    codepage: int
    #: Windows LANGID of the resources (primary | sublanguage << 10).
    resource_language: int
    #: POSIX locale under which Wine shows the program's ANSI text.
    system_locale: str
    glyphs: str
    #: US-layout character -> the character that key types.
    keyboard: dict
    messages: dict

    @property
    def keyboard_table(self) -> str:
        """The 128 characters of INPUTMGR's key translation table."""
        return ''.join(self.keyboard.get(chr(c), chr(c)) for c in range(128))


DESCRIPTOR_FIELDS = {'name': str, 'codepage': int, 'resource_language': str,
                     'system_locale': str, 'glyphs': str, 'keyboard': dict}


def _descriptor(code, text, errors):
    where = f'{LOCALES}/{code}.json'
    try:
        data = json.loads(text)
    except ValueError as exc:
        errors.append(f'{where}: {exc}')
        return None
    if not isinstance(data, dict):
        errors.append(f'{where}: expected an object')
        return None
    bad = [key for key, kind in DESCRIPTOR_FIELDS.items() if not isinstance(data.get(key), kind)]
    errors += [f'{where}: "{key}" must be a {DESCRIPTOR_FIELDS[key].__name__}' for key in bad]
    unknown = data.keys() - DESCRIPTOR_FIELDS.keys()
    if unknown:
        errors.append(f'{where}: unknown field(s) {", ".join(sorted(unknown))}')
    if bad or unknown:
        return None
    codepage, count = data['codepage'], len(errors)
    try:
        if len(bytes(range(256)).decode(f'cp{codepage}', errors='replace')) != 256:
            raise LookupError
    except LookupError:
        errors.append(f'{where}: {codepage} is not a single-byte Windows code page')
        return None
    try:
        language = int(data['resource_language'], 16)
    except ValueError:
        language = 0
    if not 0 < language < 0x10000 or language & 0x3ff == 0:
        errors.append(f'{where}: resource_language must be a Windows LANGID such as "0x0409"')
    if not re.fullmatch(r'[a-z]{2,3}_[A-Z]{2}\.UTF-8', data['system_locale']):
        errors.append(f'{where}: system_locale must be a UTF-8 POSIX locale such as "en_US.UTF-8"')
    if data['glyphs'] not in GLYPHS:
        errors.append(f'{where}: glyphs must be one of {", ".join(GLYPHS)}')
    keyboard, mapping = data['keyboard'], {}
    keys, typed = keyboard.get('keys'), keyboard.get('typed')
    if set(keyboard) - {'keys', 'typed'} or not isinstance(keys, str) \
            or not isinstance(typed, str) or len(keys) != len(typed):
        errors.append(f'{where}: keyboard needs "keys" and "typed" strings of equal length')
    else:
        for key, char in zip(keys, typed):
            if not ' ' <= key <= '~' or key in mapping:
                errors.append(f'{where}: keyboard key {key!r} is not a unique printable ASCII key')
            try:
                if len(encode(char, codepage)) != 1:
                    raise UnicodeError
            except UnicodeError:
                errors.append(f'{where}: keyboard character {char!r} is not in code page {codepage}')
            mapping[key] = char
    if len(errors) != count:
        return None
    return dict(name=data['name'], codepage=codepage, resource_language=language,
                system_locale=data['system_locale'], glyphs=data['glyphs'], keyboard=mapping)


def resource_defines(locale):
    """The RC LANGUAGE statement's operands for `locale`."""
    return (f'#define {RESOURCE_LANGUAGE} 0x{locale.resource_language & 0x3ff:02x}\n'
            f'#define {RESOURCE_SUBLANGUAGE} 0x{locale.resource_language >> 10:x}\n')


# --------------------------------------------------------------------------
# Catalog
# --------------------------------------------------------------------------

#: Validated catalogs by the digest of their files; a build parses each once.
_PARSED = {}

@dataclass(frozen=True)
class Catalog:
    ids: tuple                 # template order; descriptor IDs excluded
    entries: dict              # id -> template Entry
    locales: dict              # code -> Locale

    @staticmethod
    def files_of(repo):
        root = Path(repo) / LOCALES
        return {path.name: path.read_text(encoding='utf-8') for path in sorted(root.iterdir())
                if path.suffix in ('.pot', '.po', '.json')} if root.is_dir() else {}

    @classmethod
    def load(cls, repo):
        return cls.parse(cls.files_of(repo))

    @classmethod
    def parse(cls, files):
        """The catalog of `files` ({name in locales/: text}); invalid -> ValueError."""
        key = hashlib.sha256(json.dumps(sorted(files.items())).encode()).hexdigest()
        if key not in _PARSED:
            catalog, errors = cls.validate(files)
            if errors:
                raise ValueError('\n'.join(errors))
            _PARSED[key] = catalog
        return _PARSED[key]

    @classmethod
    def validate(cls, files):
        """(catalog or None, every error found)."""
        errors = []
        for name, text in sorted(files.items()):
            if name.endswith(('.po', '.pot')):
                errors += [f'{LOCALES}/{name}:{line}: {message}'
                           for line, message in hidden_text_errors(text, allow_unicode=True)]
        if TEMPLATE not in files:
            return None, errors + [f'{LOCALES}/{TEMPLATE} is missing']
        try:
            _header, template = parse_po(files[TEMPLATE])
        except (ValueError, SyntaxError) as exc:
            return None, errors + [f'{LOCALES}/{TEMPLATE}: {exc}']
        entries = {}
        for entry in template:
            if not ID.fullmatch(entry.msgid) or entry.msgid in entries \
                    or entry.msgid.startswith(DESCRIPTOR_PREFIX):
                errors.append(f'{LOCALES}/{TEMPLATE}: invalid or duplicate ID {entry.msgid}')
            entries[entry.msgid] = entry
        locales = {}
        for code in sorted({name.rsplit('.', 1)[0] for name in files
                            if name.endswith(('.po', '.json'))}):
            if f'{code}.json' not in files or f'{code}.po' not in files:
                errors.append(f'{LOCALES}/{code}: a language needs both {code}.po and {code}.json')
                continue
            descriptor = _descriptor(code, files[f'{code}.json'], errors)
            messages = cls._messages(code, files[f'{code}.po'], entries, errors)
            if descriptor is None or messages is None:
                continue
            locale = Locale(code=code, messages=messages, **descriptor)
            cls._check_text(locale, entries, errors)
            messages[KEYBOARD_ID] = locale.keyboard_table
            locales[code] = locale
        if not locales and not errors:
            errors.append(f'{LOCALES}: no language')
        for key, entry in entries.items():
            errors += cls._agreement(key, entry, locales)
        if errors:
            return None, errors
        return cls(tuple(entries), entries, locales), []

    @staticmethod
    def _messages(code, text, entries, errors):
        where = f'{LOCALES}/{code}.po'
        try:
            header, parsed = parse_po(text)
        except (ValueError, SyntaxError) as exc:
            errors.append(f'{where}: {exc}')
            return None
        if header.get('Language') != code:
            errors.append(f'{where}: the header Language must be {code}')
        messages, seen = {}, set()
        for entry in parsed:
            key = entry.msgid
            if key not in entries:
                errors.append(f'{where}: {key}: stale ID (not in the template)')
            elif key in seen:
                errors.append(f'{where}: {key}: duplicate entry')
            elif 'fuzzy' in entry.flags:
                errors.append(f'{where}: {key}: fuzzy translation')
            elif not entry.msgstr:
                errors.append(f'{where}: {key}: missing translation')
            else:
                messages[key] = entry.msgstr
            seen.add(key)
        missing = [key for key in entries if key not in seen]
        if missing:
            errors.append(f'{where}: {len(missing)} ID(s) without an entry: '
                          + ', '.join(missing[:8]) + (' ...' if len(missing) > 8 else ''))
        return messages

    @staticmethod
    def _check_text(locale, entries, errors):
        drawable = GLYPHS[locale.glyphs]
        for key, value in locale.messages.items():
            try:
                encode(value, locale.codepage)
            except UnicodeError as exc:
                errors.append(f'{LOCALES}/{locale.code}.po: {key}: {value[exc.start:exc.end]!r} '
                              f'is not in code page {locale.codepage}')
                continue
            if all(ref.endswith('.rc') for ref in entries[key].references):
                continue   # Windows draws resource text, not the game's fonts.
            missing = sorted({char for char in value if ord(char) >= 0x80 and char not in drawable})
            if missing:
                errors.append(f'{LOCALES}/{locale.code}.po: {key}: no game-font glyph for '
                              f'{"".join(missing)!r} (glyphs "{locale.glyphs}")')

    @staticmethod
    def _agreement(key, entry, locales):
        """Every language gives `key` the same printf arguments and, for a
        fixed-width field, the same byte length."""
        errors = []
        signatures, widths = {}, {}
        for code, locale in locales.items():
            value = locale.messages.get(key)
            if value is None:
                continue
            signatures.setdefault(tuple(format_signature(value)), []).append(code)
            try:
                widths.setdefault(len(encode(value, locale.codepage)), []).append(code)
            except UnicodeError:
                pass
        if len(signatures) > 1:
            errors.append(f'{key}: printf arguments differ: ' + '; '.join(
                f'{",".join(codes)} {" ".join("%" + s for s in signature) or "(none)"}'
                for signature, codes in signatures.items()))
        if FIXED_WIDTH_NOTE in entry.notes and len(widths) > 1:
            errors.append(f'{key}: fixed-width text differs in byte length: ' + '; '.join(
                f'{",".join(codes)} {width}' for width, codes in sorted(widths.items())))
        return errors

    # -- rendering ---------------------------------------------------------

    def locale(self, code):
        if code not in self.locales:
            raise ValueError(f'unsupported locale: {code} (have {", ".join(self.locales)})')
        return self.locales[code]

    def messages(self, locale):
        return self.locale(locale).messages

    def macro(self, key, *, chars=False):
        if key not in self.entries and key != KEYBOARD_ID:
            raise ValueError(f'unknown localization ID: {key}')
        return ('H1C' if chars else 'H1L') + hashlib.sha256(key.encode('ascii')).hexdigest()[:8]

    def header(self, locale, *, character_keys=()):
        current = self.locale(locale)
        keys = sorted(self.entries)
        macros = [self.macro(key) for key in [*keys, KEYBOARD_ID]]
        if len(set(macros)) != len(macros):
            raise ValueError('localization macro hash collision')
        text = ''.join(f'#define {self.macro(key)} '
                       f'{literal(current.messages[key], current.codepage)}\n' for key in keys)
        for key in sorted(set(character_keys)):
            text += (f'#define {self.macro(key, chars=True)} '
                     f'{character_initializer(current.messages[key], current.codepage)}\n')
        return text

    def render_resource(self, text, *, locale):
        """RC input with Unicode literals and the selected Windows language.

        Numeric narrow escapes in RC strings represent Unicode code points,
        unlike C++ code page byte literals. Generate explicit wide strings from
        the same catalog, without embedding translated text in authored RC.
        """
        current = self.locale(locale)
        rendered = text
        for start, end, key, method in reversed(list(self.typed_calls(text))):
            if method != 'Tr':
                raise ValueError('character arrays are not resource strings')
            rendered = rendered[:start] + resource_literal(current.messages[key]) + rendered[end:]
        return resource_defines(current) + rendered

    def calls(self, text):
        for start, end, key, _method in self.typed_calls(text):
            yield start, end, key

    def typed_calls(self, text):
        for start, end, key, method in find_calls(text):
            self.macro(key)
            yield start, end, key, method

    def render(self, text, *, locale, expanded=False):
        current = self.locale(locale)
        out = text
        replacements = []
        for start, end, key, method in self.typed_calls(text):
            chars = method == 'Chars'
            if expanded:
                value = current.messages[key]
                replacement = (character_initializer(value, current.codepage) if chars
                               else literal(value, current.codepage))
            else:
                # Macro names are shorter than "localization". Padding retains
                # original UTF-8 byte positions, even for multiline calls.
                replacement = self.macro(key, chars=chars)
            replacements.append((start, end, replacement))
        for start, end, replacement in sorted(replacements, reverse=True):
            if not expanded:
                old = text[start:end].encode('utf-8')
                replacement += ''.join('\n' if c == 10 else ' '
                                       for c in old[len(replacement.encode('utf-8')):])
            out = out[:start] + replacement + out[end:]
        return out


# --------------------------------------------------------------------------
# Template and .po maintenance
# --------------------------------------------------------------------------

TEMPLATE_PREAMBLE = (
    'Message template: one entry per localization::Tr/Chars ID in src/ and',
    'include/. Generated by `homm1 localization update`; do not edit.',
)


def scan(root):
    """{id: {'files': [...], 'chars': bool}} over src/ and include/."""
    found = {}
    for path in source_files(root):
        name = path.relative_to(root).as_posix()
        for _start, _end, key, method in find_calls(path.read_text(encoding='utf-8')):
            record = found.setdefault(key, {'files': [], 'chars': False})
            if name not in record['files']:
                record['files'].append(name)
                record['files'].sort()
            record['chars'] |= method == 'Chars'
    return found


def id_order(key):
    """IDs sort by name, numbered parts by number: moving a call in the source
    leaves the template alone."""
    return [(0, int(part), '') if part.isdigit() else (1, 0, part) for part in key.split('.')]


def template_text(found, reference=None):
    """The template for scan() results. `reference` ({id: text} of the
    matching language) marks the printf formats for translation tools."""
    entries = []
    for key, record in sorted(found.items(), key=lambda item: id_order(item[0])):
        if key.startswith(DESCRIPTOR_PREFIX):
            continue
        entry = Entry(key, references=list(record['files']))
        if record['chars']:
            entry.notes.append(FIXED_WIDTH_NOTE)
        text = (reference or {}).get(key, '')
        if format_signature(text) or '%%' in text:
            entry.flags.append('c-format')
        entries.append(entry)
    return write_po([('Content-Type', 'text/plain; charset=UTF-8')], TEMPLATE_PREAMBLE, entries)


def po_text(code, template, existing):
    """(text, stale IDs): `existing` (a parse_po result, or None) in the
    template's order with its comments; translations and translator comments
    are kept, IDs the template no longer has are dropped."""
    _header, entries = existing or ({}, [])
    old = {entry.msgid: entry for entry in entries}
    out = []
    for item in template:
        entry = Entry(item.msgid, references=item.references, flags=item.flags, notes=item.notes)
        if item.msgid in old:
            entry.msgstr, entry.comments = old[item.msgid].msgstr, old[item.msgid].comments
            entry.flags = [*entry.flags, *(f for f in old[item.msgid].flags if f == 'fuzzy')]
        out.append(entry)
    stale = sorted(old.keys() - {item.msgid for item in template})
    return write_po([('Language', code), ('Content-Type', 'text/plain; charset=UTF-8')],
                    (), out), stale


def matching_locale(root):
    """The retail program's language: the pinned target's (a reconstruction
    checkout) or the build manifest's (a generated source tree)."""
    root = Path(root)
    target, manifest = root / 'config/retail/targets.json', root / 'build.json'
    if target.is_file():
        return json.loads(target.read_text())['game'].get('locale', 'ru')
    if manifest.is_file():
        return json.loads(manifest.read_text()).get('locale', 'ru')
    return None


def update(root, reference_locale, *, check=False):
    """Regenerate locales/messages.pot from the source and rewrite every .po in
    its order. Returns (changed file names, {code: stale IDs dropped})."""
    root = Path(root)
    directory = root / LOCALES
    reference = None
    po = directory / f'{reference_locale}.po'
    if reference_locale and po.is_file():
        reference = {e.msgid: e.msgstr for e in parse_po(po.read_text(encoding='utf-8'))[1]}
    outputs = {TEMPLATE: template_text(scan(root), reference)}
    template = parse_po(outputs[TEMPLATE])[1]
    stale = {}
    for descriptor in sorted(directory.glob('*.json')):
        code = descriptor.stem
        path = directory / f'{code}.po'
        existing = parse_po(path.read_text(encoding='utf-8')) if path.is_file() else None
        outputs[f'{code}.po'], stale[code] = po_text(code, template, existing)
    changed = [name for name, text in outputs.items()
               if not (directory / name).is_file()
               or (directory / name).read_text(encoding='utf-8') != text]
    if not check:
        for name in changed:
            (directory / name).write_text(outputs[name], encoding='utf-8')
    return changed, {code: ids for code, ids in stale.items() if ids}


def check(root, reference_locale=None):
    """Every localization error of the tree at `root` (an empty list passes):
    the catalogs, the template's freshness, and the authored source text."""
    root = Path(root)
    catalog, errors = Catalog.validate(Catalog.files_of(root))
    if reference_locale and catalog is not None and reference_locale not in catalog.locales:
        errors.append(f'{LOCALES}: the matching language {reference_locale} has no catalog')
    try:
        changed, _stale = update(root, reference_locale, check=True)
        errors += [f'{LOCALES}/{name} is out of date; run `homm1 localization update`'
                   for name in changed]
    except (OSError, ValueError, SyntaxError) as exc:
        errors.append(f'{LOCALES}: {exc}')
    for path in source_files(root):
        text = path.read_text(encoding='utf-8')
        name = path.relative_to(root).as_posix()
        errors += [f'{name}:{line}: {message}' for line, message in hidden_text_errors(text)]
        try:
            for _start, _end, key, method in find_calls(text):
                if key.startswith(DESCRIPTOR_PREFIX) and (key != KEYBOARD_ID or method != 'Chars'):
                    errors.append(f'{name}: {key}: descriptors supply only '
                                  f'localization::Chars("{KEYBOARD_ID}")')
        except ValueError as exc:
            errors.append(f'{name}: {exc}')
    return errors


try:
    from homm1.core.usage import logged
except ImportError:   # the generated source tree carries this module alone
    def logged(function):
        return function


@logged
def main(argv=None):
    import argparse
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', choices=('check', 'update'))
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument('--check', action='store_true',
                        help='update: change nothing; fail when a file is out of date')
    args = parser.parse_args(argv)
    return run(args.command, args.root, matching_locale(args.root), check_only=args.check)


def run(command, root, reference, *, check_only=False, prefix='[localization]'):
    if command == 'check':
        errors = check(root, reference)
        for error in errors:
            print(error)
        print(f'{prefix} {len(errors)} error(s)')
        return int(bool(errors))
    changed, stale = update(root, reference, check=check_only)
    for code, ids in stale.items():
        print(f'{prefix} {code}.po: {"would drop" if check_only else "dropped"} '
              f'{len(ids)} stale ID(s): {", ".join(ids)}')
    for name in changed:
        print(f'{prefix} {"out of date" if check_only else "wrote"}: {LOCALES}/{name}')
    if not changed:
        print(f'{prefix} {LOCALES}/ is up to date')
    return int(bool(check_only and changed))


if __name__ == '__main__':
    raise SystemExit(main())
