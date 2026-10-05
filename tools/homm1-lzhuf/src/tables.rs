//! Codec constants and the fixed tables shared with `HEROES.EXE`.
//!
//! The values follow `vendor/lzhuf/encoder.cpp`, whose typed initializers are
//! proven against the retail image. `tests/tables.rs` parses that source and
//! checks every table here, including the initial Huffman tree that
//! [`crate::huffman`] builds instead of storing.

/// Sliding-window size (`WINDOW_SIZE`, Okumura's `N`).
pub const WINDOW_SIZE: usize = 4096;
/// Longest match, also the look-ahead length (`LOOK_AHEAD`, Okumura's `F`).
pub const LOOK_AHEAD: usize = 60;
/// Matches of this length or shorter are coded as literals.
pub const MATCH_THRESHOLD: usize = 2;
/// Window bytes the encoder presets to spaces before reading input.
pub const PREFILL: usize = WINDOW_SIZE - LOOK_AHEAD;
/// Size of the game's `text_buf`, including the encoder's look-ahead mirror.
pub const WINDOW_BYTES: usize = WINDOW_SIZE + LOOK_AHEAD - 1;
/// Byte the encoder writes over `text_buf[..PREFILL]`.
pub const PREFILL_BYTE: u8 = b' ';
/// Big-endian uncompressed-length prefix.
pub const HEADER_SIZE: usize = 4;

/// Literal bytes plus match lengths `MATCH_THRESHOLD + 1 ..= LOOK_AHEAD`.
pub(crate) const CHARACTER_COUNT: usize = 256 - MATCH_THRESHOLD + LOOK_AHEAD;
/// Nodes in the adaptive Huffman tree.
pub(crate) const TREE_SIZE: usize = CHARACTER_COUNT * 2 - 1;
/// Root node index.
pub(crate) const ROOT: usize = TREE_SIZE - 1;
/// Root frequency that triggers a tree rebuild before the next update.
pub(crate) const MAX_FREQUENCY: u16 = 0x8000;
/// Sentinel stored after the last frequency (`initialFrequency[627]`).
pub(crate) const FREQUENCY_SENTINEL: u16 = 0xffff;
/// "No node" in the encoder's binary search trees.
pub(crate) const NIL: usize = WINDOW_SIZE;

/// Encoder: code length of the upper six position bits (`positionLength`).
pub const POSITION_LENGTH: [u8; 64] = [
    0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
];

/// Encoder: left-aligned code of the upper six position bits (`positionCode`).
pub const POSITION_CODE: [u8; 64] = [
    0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
    0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC, 0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
    0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE, 0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
];

/// Decoder: upper six position bits, indexed by the next eight code bits
/// (`d_code`).
pub const D_CODE: [u8; 256] = [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9,
    10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13,
    14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19,
    20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27,
    28, 28, 29, 29, 30, 30, 31, 31, 32, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 38, 39, 39,
    40, 40, 41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 46, 46, 47, 47, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, 62, 63,
];

/// Decoder: total code length of a position whose first eight bits index
/// this table (`d_len`).
pub const D_LEN: [u8; 256] = [
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
];

/// Exposes the fixed tables to the integration tests, which compare them with
/// the reconstruction's initializers.
#[doc(hidden)]
pub mod raw {
    pub use super::{D_CODE, D_LEN, POSITION_CODE, POSITION_LENGTH};

    /// The `(son, frequency, parent)` arrays both game entry points copy from
    /// `initialSon`, `initialFrequency` and `initialParent`.
    pub fn initial_tree() -> (Vec<u16>, Vec<u16>, Vec<u16>) {
        let tree = crate::huffman::AdaptiveTree::new();
        (tree.son.to_vec(), tree.freq.to_vec(), tree.parent.to_vec())
    }
}
