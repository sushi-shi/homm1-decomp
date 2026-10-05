//! Byte-exact reimplementation of the LZHUF codec in Heroes of Might and
//! Magic (1995), as linked into the Buka 2003 `HEROES.EXE`.
//!
//! The game uses the codec for one purpose: `TransmitSaveGame` compresses
//! `REMOTE.GAM` before sending it to a network opponent, and the receiver
//! decodes it. The algorithm is Okumura's LZHUF (4096-byte window, 60-byte
//! look-ahead, adaptive Huffman coding of 314 literal/length symbols, a fixed
//! prefix code for the upper six position bits), in a variant with these
//! game-specific properties:
//!
//! * A stream is the uncompressed length as a big-endian `u32`, then the
//!   code bits, most significant first, with the last byte zero-padded.
//!   There is no end-of-stream symbol. `EncodeData` returns the length of the
//!   code bytes only, without the four-byte prefix.
//! * Among equally long matches the encoder picks the nearest.
//! * The window (`text_buf`) is a global that both directions share and
//!   never clear. The encoder presets its first [`PREFILL`] bytes to spaces,
//!   but its search trees also read the rest of the window, which still holds
//!   earlier data. The decoder presets nothing. [`Codec`] models that state;
//!   [`compress`] starts from a fresh process and [`decompress`] from the
//!   window the encoder assumes.
//! * The encoder's look-ahead counter is an unsigned short. For empty input
//!   it wraps, and the encoder codes 65 536 window positions anyway; the
//!   decoder reads none of them because the declared length is zero.
//! * Huffman codes are assembled in 16 bits. A path deeper than sixteen
//!   nodes (possible only with extremely skewed frequencies) keeps its last
//!   sixteen branch bits, as in the game, and the decoder walks the tree
//!   bit by bit.
//!
//! Every behaviour here is checked against the retail machine code; see the
//! crate README.

#![forbid(unsafe_code)]

mod decoder;
mod encoder;
mod huffman;
mod tables;

pub use tables::raw;
pub use tables::{
    HEADER_SIZE, LOOK_AHEAD, MATCH_THRESHOLD, PREFILL, PREFILL_BYTE, WINDOW_BYTES, WINDOW_SIZE,
};

use std::fmt;

/// Codec failures. Encoding fails only for inputs whose length does not fit
/// the four-byte prefix.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Error {
    /// The input is longer than `u32::MAX` bytes.
    InputTooLarge { length: usize },
    /// The stream is shorter than its four-byte length prefix.
    MissingHeader { length: usize },
    /// The code bits ended before the declared number of bytes was decoded.
    Truncated { declared: u32, decoded: usize },
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::InputTooLarge { length } => {
                write!(
                    f,
                    "input of {length} bytes exceeds the 32-bit length prefix"
                )
            }
            Self::MissingHeader { length } => {
                write!(f, "stream of {length} bytes has no four-byte length prefix")
            }
            Self::Truncated { declared, decoded } => write!(
                f,
                "stream ends after {decoded} of {declared} declared bytes"
            ),
        }
    }
}

impl std::error::Error for Error {}

/// The codec's persistent state: the game's shared `text_buf` window.
///
/// Every other global is reset by each `EncodeData`/`DecodeData` call, so a
/// sequence of calls on one `Codec` reproduces the same sequence of calls in
/// one game process.
#[derive(Clone)]
pub struct Codec {
    window: Box<[u8; WINDOW_BYTES]>,
}

impl Codec {
    /// The window of a freshly started game process (all zero).
    pub fn new() -> Self {
        Self {
            window: Box::new([0; WINDOW_BYTES]),
        }
    }

    /// A window whose first [`PREFILL`] bytes are spaces, as the encoder
    /// assumes when it emits matches into the preset region.
    pub fn with_encoder_prefill() -> Self {
        let mut codec = Self::new();
        codec.window[..PREFILL].fill(PREFILL_BYTE);
        codec
    }

    /// A codec whose window starts with `bytes`; the rest is zero.
    ///
    /// # Panics
    ///
    /// If `bytes` is longer than [`WINDOW_BYTES`].
    pub fn with_window(bytes: &[u8]) -> Self {
        let mut codec = Self::new();
        codec.window[..bytes.len()].copy_from_slice(bytes);
        codec
    }

    /// The current window contents.
    pub fn window(&self) -> &[u8; WINDOW_BYTES] {
        &self.window
    }

    /// `EncodeData`: compresses `input` into a complete stream, length prefix
    /// included, and leaves the window as the game does.
    pub fn encode(&mut self, input: &[u8]) -> Result<Vec<u8>, Error> {
        encoder::encode(&mut self.window, input)
    }

    /// `DecodeData`: decompresses a complete stream against the current
    /// window and leaves the window as the game does.
    ///
    /// Bytes the game would read past the end of the stream are taken as
    /// zero; a stream that needs more than that is reported as truncated.
    pub fn decode(&mut self, stream: &[u8]) -> Result<Vec<u8>, Error> {
        decoder::decode(&mut self.window, stream)
    }
}

impl Default for Codec {
    fn default() -> Self {
        Self::new()
    }
}

impl fmt::Debug for Codec {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Codec").finish_non_exhaustive()
    }
}

/// Compresses `input` as a freshly started game process would.
pub fn compress(input: &[u8]) -> Result<Vec<u8>, Error> {
    Codec::new().encode(input)
}

/// Decompresses a stream produced by the game's encoder (or [`compress`]).
///
/// The window starts with the encoder's space prefill, which makes this the
/// exact inverse of [`compress`]. The game's own receiver decodes against
/// whatever its window holds; use [`Codec`] to reproduce that.
pub fn decompress(stream: &[u8]) -> Result<Vec<u8>, Error> {
    Codec::with_encoder_prefill().decode(stream)
}

/// The uncompressed length a stream declares.
pub fn declared_length(stream: &[u8]) -> Result<u32, Error> {
    stream
        .get(..HEADER_SIZE)
        .and_then(|bytes| bytes.try_into().ok())
        .map(u32::from_be_bytes)
        .ok_or(Error::MissingHeader {
            length: stream.len(),
        })
}
