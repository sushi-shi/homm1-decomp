"""Portable static localization catalog; shared with generated source builds."""
from __future__ import annotations

import ast
import hashlib
import json
import re
from dataclasses import dataclass
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


def tokens(text):
    return [m for m in TOKEN.finditer(text) if m.lastgroup not in ('space', 'comment')]


def quoted(value):
    return json.dumps(value, ensure_ascii=False)


def literal(value: str) -> str:
    """ASCII C literal encoding exact Windows-1251 bytes (fixed-width octal)."""
    out = []
    for byte in value.encode('cp1251'):
        if byte in (34, 92):
            out.append('\\' + chr(byte))
        elif 32 <= byte < 127:
            out.append(chr(byte))
        else:
            out.append('\\%03o' % byte)
    return '"' + ''.join(out) + '"'


def character_initializer(value: str) -> str:
    """A fixed-width char-array initializer, without an implicit terminator."""
    result = []
    for byte in value.encode('cp1251'):
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


def parse_registry(text):
    ts = tokens(text)
    entries = {}
    i = 0
    while i < len(ts):
        if ts[i].group() != 'HOMM1_MESSAGE' or i + 4 >= len(ts):
            raise ValueError('expected HOMM1_MESSAGE(id, English text)')
        if ts[i + 1].group() != '(' or ts[i + 3].group() != ',':
            raise ValueError('malformed HOMM1_MESSAGE')
        key = ast.literal_eval(ts[i + 2].group())
        i += 4
        parts = []
        while i < len(ts) and ts[i].lastgroup == 'string':
            parts.append(ast.literal_eval(ts[i].group()))
            i += 1
        if not parts or i >= len(ts) or ts[i].group() != ')':
            raise ValueError(f'{key}: expected English string literal')
        if not ID.fullmatch(key) or key in entries:
            raise ValueError(f'invalid or duplicate message ID: {key}')
        entries[key] = ''.join(parts)
        i += 1
    return entries


def parse_po(text):
    entries, fields, active, fuzzy = [], {}, None, False
    for line in [*text.splitlines(), '']:
        line = line.strip()
        if not line:
            if fields:
                if fuzzy:
                    raise ValueError('fuzzy translations are not allowed in the retail catalog')
                entries.append(fields)
            fields, active, fuzzy = {}, None, False
        elif line.startswith('#,'):
            fuzzy |= 'fuzzy' in line[2:].split(',') or 'fuzzy' in line[2:].split()
        elif line.startswith('#'):
            continue
        elif line.startswith('"') and active:
            fields[active] += ast.literal_eval(line)
        else:
            match = re.fullmatch(r'(msgctxt|msgid|msgstr)\s+(".*")', line)
            if match is None or match[1] in fields:
                raise ValueError(f'unsupported or duplicate PO field: {line}')
            active = match[1]
            fields[active] = ast.literal_eval(match[2])
    return entries


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


@dataclass(frozen=True)
class Catalog:
    english: dict[str, str]
    russian: dict[str, str]

    @classmethod
    def load(cls, repo):
        root = Path(repo) / 'locales'
        variants_path = root / 'format-variants.json'
        return cls.parse((root / 'messages.def').read_text(encoding='utf-8'),
                         (root / 'ru.po').read_text(encoding='utf-8'),
                         variants_path.read_text() if variants_path.is_file() else None)

    @classmethod
    def parse(cls, registry_text, po_text, variants_text=None):
        """The catalog from the texts of messages.def, ru.po and the optional
        format-variants.json (a snapshot that is not a checkout)."""
        for name, text in (('messages.def', registry_text), ('ru.po', po_text)):
            errors = hidden_text_errors(text, allow_unicode=True)
            if errors:
                raise ValueError(f'{name}:{errors[0][0]}: {errors[0][1]}')
        english = parse_registry(registry_text)
        variants = json.loads(variants_text) if variants_text is not None else {}
        if not isinstance(variants, dict) or variants.keys() - english.keys():
            raise ValueError('unknown format variant ID')
        for key, signatures in variants.items():
            if (not isinstance(signatures, dict) or set(signatures) != {'en', 'ru'} or
                    any(not isinstance(v, list) or not all(isinstance(t, str) for t in v)
                        for v in signatures.values()) or signatures['en'] == signatures['ru']):
                raise ValueError(f'{key}: invalid format variant signatures')
        russian = {}
        for entry in parse_po(po_text):
            key = entry.get('msgctxt')
            if key is None and entry.get('msgid') == '':
                continue
            if key not in english or key in russian or entry.get('msgid') != english[key]:
                raise ValueError(f'{key}: unknown, duplicate or stale PO entry')
            value = entry.get('msgstr')
            if not value:
                raise ValueError(f'{key}: missing Russian translation')
            signatures = {'en': format_signature(english[key]), 'ru': format_signature(value)}
            if key in variants:
                if signatures != variants[key]:
                    raise ValueError(f'{key}: stale format variant signatures')
            elif signatures['ru'] != signatures['en']:
                raise ValueError(f'{key}: English/Russian printf placeholders differ')
            literal(value)  # Fail on unrepresentable Unicode, never replace it.
            russian[key] = value
        if russian.keys() != english.keys():
            raise ValueError('missing Russian translations: ' + ', '.join(english.keys() - russian.keys()))
        return cls(english, russian)

    def macro(self, key, *, chars=False):
        if key not in self.russian:
            raise ValueError(f'unknown localization ID: {key}')
        return ('H1C' if chars else 'H1L') + hashlib.sha256(key.encode('ascii')).hexdigest()[:8]

    def messages(self, locale='ru'):
        if locale not in ('ru', 'en'):
            raise ValueError(f'unsupported locale: {locale}')
        return self.russian if locale == 'ru' else self.english

    def header(self, locale='ru', *, character_keys=()):
        macros = [self.macro(key) for key in self.russian]
        if len(set(macros)) != len(macros):
            raise ValueError('localization macro hash collision')
        messages = self.messages(locale)
        text = ''.join(f'#define {self.macro(key)} {literal(value)}\n'
                       for key, value in sorted(messages.items()))
        for key in sorted(set(character_keys)):
            text += (f'#define {self.macro(key, chars=True)} '
                     f'{character_initializer(messages[key])}\n')
        return text

    def render_resource(self, text, *, locale='ru'):
        """RC input with Unicode literals and the selected Windows language.

        Numeric narrow escapes in RC strings represent Unicode code points,
        unlike C++ CP1251 byte literals. Generate explicit wide strings from
        the same catalog, without embedding translated text in authored RC.
        """
        messages = self.messages(locale)
        rendered = text
        for start, end, key, method in reversed(list(self.typed_calls(text))):
            if method != 'Tr':
                raise ValueError('character arrays are not resource strings')
            rendered = rendered[:start] + resource_literal(messages[key]) + rendered[end:]
        language = 0x19 if locale == 'ru' else 0x09
        return f'#define HOMM1_RESOURCE_LANGUAGE 0x{language:02x}\n' + rendered

    def calls(self, text):
        for start, end, key, _method in self.typed_calls(text):
            yield start, end, key

    def typed_calls(self, text):
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
            self.macro(key)
            yield token.start(), tail[5].end(), key, tail[2].group()

    def render(self, text, *, expanded=False, locale='ru'):
        messages = self.messages(locale)
        out = text
        replacements = [(t.start(), t.end(), '1' if locale == 'ru' else '0')
                        for t in tokens(text)
                        if t.lastgroup == 'identifier' and t.group() == 'HOMM1_RUSSIAN']
        for start, end, key, method in self.typed_calls(text):
            chars = method == 'Chars'
            if expanded:
                replacement = (character_initializer(messages[key]) if chars
                               else literal(messages[key]))
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


def hidden_text_errors(text, *, allow_unicode=False):
    """Reject numeric escapes; spell controls as \n, \t, etc. (NUL as \0).

    Escaped backslashes are consumed as a pair, so a path containing the text
    ``\\\\x`` is not mistaken for a byte escape. Checks strings AND characters.
    Cyrillic belongs in the catalog, not in inline game literals.
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
