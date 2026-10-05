//! `EncodeData`: LZSS over binary search trees, coded with the adaptive tree.

use crate::huffman::AdaptiveTree;
use crate::tables::{
    HEADER_SIZE, LOOK_AHEAD, MATCH_THRESHOLD, NIL, POSITION_CODE, POSITION_LENGTH, PREFILL,
    PREFILL_BYTE, ROOT, WINDOW_BYTES, WINDOW_SIZE,
};
use crate::Error;

const MASK: usize = WINDOW_SIZE - 1;
/// Root of the search tree for strings starting with byte `b` is
/// `ROOTS + b`.
const ROOTS: usize = WINDOW_SIZE + 1;

/// Encodes `input` against the shared window, as `EncodeData` does.
pub(crate) fn encode(window: &mut [u8; WINDOW_BYTES], input: &[u8]) -> Result<Vec<u8>, Error> {
    let source_length = u32::try_from(input.len()).map_err(|_| Error::InputTooLarge {
        length: input.len(),
    })?;
    let mut writer = BitWriter::new(input.len());
    writer.out.extend_from_slice(&source_length.to_be_bytes());

    let mut tree = AdaptiveTree::new();
    let mut matcher = Matcher::new();
    let mut source = input.iter().copied();

    let mut s = 0;
    let mut r = PREFILL;
    window[..r].fill(PREFILL_BYTE);
    // `len` is the game's unsigned short look-ahead count. Empty input makes
    // it wrap below zero; the game then codes 65 536 window positions before
    // stopping, and so does this port.
    let mut len: u16 = 0;
    while usize::from(len) < LOOK_AHEAD {
        let Some(byte) = source.next() else { break };
        window[r + usize::from(len)] = byte;
        len += 1;
    }
    for back in 1..=LOOK_AHEAD {
        matcher.insert(window, r - back);
    }
    matcher.insert(window, r);

    loop {
        if i32::from(matcher.match_length) > i32::from(len) {
            matcher.match_length = len as i16;
        }
        if matcher.match_length <= MATCH_THRESHOLD as i16 {
            matcher.match_length = 1;
            encode_symbol(&mut tree, &mut writer, usize::from(window[r]));
        } else {
            let length = matcher.match_length as usize;
            encode_symbol(&mut tree, &mut writer, 255 - MATCH_THRESHOLD + length);
            encode_position(&mut writer, matcher.match_position);
        }

        let last_match_length = matcher.match_length;
        let mut consumed: i16 = 0;
        while consumed < last_match_length {
            let Some(byte) = source.next() else { break };
            matcher.delete(s);
            window[s] = byte;
            if s < LOOK_AHEAD - 1 {
                window[s + WINDOW_SIZE] = byte;
            }
            s = (s + 1) & MASK;
            r = (r + 1) & MASK;
            matcher.insert(window, r);
            consumed += 1;
        }
        while consumed < last_match_length {
            consumed += 1;
            matcher.delete(s);
            s = (s + 1) & MASK;
            r = (r + 1) & MASK;
            len = len.wrapping_sub(1);
            if len != 0 {
                matcher.insert(window, r);
            }
        }
        if len == 0 {
            break;
        }
    }

    writer.finish();
    Ok(writer.out)
}

/// `EncodeCharacter`: emits the path from the symbol's leaf to the root, then
/// updates the tree.
fn encode_symbol(tree: &mut AdaptiveTree, writer: &mut BitWriter, symbol: usize) {
    // The game assembles the path in an unsigned short, so a path deeper than
    // sixteen nodes keeps only its last sixteen branch bits.
    let mut code: u16 = 0;
    let mut length: i16 = 0;
    let mut node = tree.leaf(symbol);
    loop {
        code >>= 1;
        if node & 1 != 0 {
            code += 0x8000;
        }
        length += 1;
        node = usize::from(tree.parent[node]);
        if node == ROOT {
            break;
        }
    }
    writer.put(length, code);
    tree.update(symbol);
}

/// `EncodePosition`: six upper bits through the fixed prefix table, six lower
/// bits verbatim.
fn encode_position(writer: &mut BitWriter, position: u16) {
    let upper = usize::from(position >> 6);
    writer.put(
        i16::from(POSITION_LENGTH[upper]),
        u16::from(POSITION_CODE[upper]) << 8,
    );
    writer.put(6, (position & 0x3f) << 10);
}

/// `PutCode` state: up to sixteen pending bits, most significant first.
struct BitWriter {
    out: Vec<u8>,
    buffer: u16,
    length: u8,
}

impl BitWriter {
    fn new(capacity: usize) -> Self {
        Self {
            out: Vec::with_capacity(HEADER_SIZE + capacity + capacity / 8 + 16),
            buffer: 0,
            length: 0,
        }
    }

    /// Appends the top `length` bits of `code`.
    ///
    /// The arithmetic mirrors the compiled `PutCode`: the code is promoted to
    /// a 32-bit integer, x86 shifts use the count modulo 32, and the results
    /// are truncated to the 16-bit buffer and 8-bit length. For codes of at
    /// most sixteen bits this is ordinary MSB-first bit packing.
    fn put(&mut self, length: i16, code: u16) {
        let wide = u32::from(code);
        self.buffer |= wide.wrapping_shr(u32::from(self.length)) as u16;
        self.length = self.length.wrapping_add(length as u8);
        if self.length >= 8 {
            self.out.push((self.buffer >> 8) as u8);
            self.length -= 8;
            if self.length >= 8 {
                self.out.push(self.buffer as u8);
                self.length -= 8;
                let shift = i32::from(length) - i32::from(self.length);
                self.buffer = wide.wrapping_shl(shift as u32) as u16;
            } else {
                self.buffer <<= 8;
            }
        }
    }

    /// `EncodeEnd`: flushes a partial final byte.
    fn finish(&mut self) {
        if self.length != 0 {
            self.out.push((self.buffer >> 8) as u8);
        }
    }
}

/// `InsertNode`/`DeleteNode` state: one binary search tree per first byte
/// over the window positions that start a string.
struct Matcher {
    left: Box<[u16; WINDOW_SIZE + 1]>,
    right: Box<[u16; WINDOW_SIZE + 257]>,
    parent: Box<[u16; WINDOW_SIZE + 1]>,
    match_position: u16,
    match_length: i16,
}

impl Matcher {
    /// `InitializeTree`: empty roots, no position in any tree.
    fn new() -> Self {
        Self {
            left: Box::new([NIL as u16; WINDOW_SIZE + 1]),
            right: Box::new([NIL as u16; WINDOW_SIZE + 257]),
            parent: Box::new([NIL as u16; WINDOW_SIZE + 1]),
            match_position: 0,
            match_length: 0,
        }
    }

    fn left(&self, node: usize) -> usize {
        usize::from(self.left[node])
    }

    fn right(&self, node: usize) -> usize {
        usize::from(self.right[node])
    }

    fn parent(&self, node: usize) -> usize {
        usize::from(self.parent[node])
    }

    /// Inserts the string at `node`, recording the longest earlier match.
    /// Among equally long matches the nearest wins. A full-length match
    /// replaces the older node in the tree.
    fn insert(&mut self, window: &[u8; WINDOW_BYTES], node: usize) {
        let mut cmp: i32 = 1;
        let mut p = ROOTS + usize::from(window[node]);
        self.right[node] = NIL as u16;
        self.left[node] = NIL as u16;
        self.match_length = 0;
        loop {
            if cmp >= 0 {
                if self.right(p) == NIL {
                    self.right[p] = node as u16;
                    self.parent[node] = p as u16;
                    return;
                }
                p = self.right(p);
            } else {
                if self.left(p) == NIL {
                    self.left[p] = node as u16;
                    self.parent[node] = p as u16;
                    return;
                }
                p = self.left(p);
            }

            let mut i = 1;
            while i < LOOK_AHEAD {
                cmp = i32::from(window[node + i]) - i32::from(window[p + i]);
                if cmp != 0 {
                    break;
                }
                i += 1;
            }
            if i > MATCH_THRESHOLD {
                let length = i as i16;
                let distance = (((node + WINDOW_SIZE - p) & MASK) - 1) as u16;
                if length > self.match_length {
                    self.match_position = distance;
                    self.match_length = length;
                    if i >= LOOK_AHEAD {
                        break;
                    }
                }
                if length == self.match_length && distance < self.match_position {
                    self.match_position = distance;
                }
            }
        }

        self.parent[node] = self.parent[p];
        self.left[node] = self.left[p];
        self.right[node] = self.right[p];
        let (left, right, up) = (self.left(p), self.right(p), self.parent(p));
        self.parent[left] = node as u16;
        self.parent[right] = node as u16;
        if self.right(up) == p {
            self.right[up] = node as u16;
        } else {
            self.left[up] = node as u16;
        }
        self.parent[p] = NIL as u16;
    }

    /// Removes the string at `node` from its tree, if present.
    fn delete(&mut self, node: usize) {
        if self.parent(node) == NIL {
            return;
        }
        let replacement;
        if self.right(node) == NIL {
            replacement = self.left(node);
        } else if self.left(node) == NIL {
            replacement = self.right(node);
        } else {
            let mut q = self.left(node);
            if self.right(q) != NIL {
                while self.right(q) != NIL {
                    q = self.right(q);
                }
                let (q_left, q_up) = (self.left(q), self.parent(q));
                self.right[q_up] = q_left as u16;
                self.parent[q_left] = q_up as u16;
                self.left[q] = self.left[node];
                let node_left = self.left(node);
                self.parent[node_left] = q as u16;
            }
            self.right[q] = self.right[node];
            let node_right = self.right(node);
            self.parent[node_right] = q as u16;
            replacement = q;
        }
        let up = self.parent(node);
        self.parent[replacement] = up as u16;
        if self.right(up) == node {
            self.right[up] = replacement as u16;
        } else {
            self.left[up] = replacement as u16;
        }
        self.parent[node] = NIL as u16;
    }
}
