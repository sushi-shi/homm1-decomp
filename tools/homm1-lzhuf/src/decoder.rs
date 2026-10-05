//! `DecodeData` and the assembly decoder (`Decode`, `GetBit`,
//! `DecodePosition`).

use crate::huffman::AdaptiveTree;
use crate::tables::{
    D_CODE, D_LEN, HEADER_SIZE, LOOK_AHEAD, MATCH_THRESHOLD, PREFILL, ROOT, TREE_SIZE,
    WINDOW_BYTES, WINDOW_SIZE,
};
use crate::Error;

const MASK: usize = WINDOW_SIZE - 1;

/// Bytes the game's bit reader may fetch past the last byte it needs. Both
/// `GetBit` and `DecodePosition` refill whole bytes while eight or fewer bits
/// are buffered, so they can read at most one byte that carries no needed
/// bit. Reads past the end of the stream return zero up to this allowance;
/// one more read means the stream is truncated.
const READ_AHEAD: usize = 2;

/// Decodes a complete stream (length prefix included) against the shared
/// window, as `DecodeData` does.
pub(crate) fn decode(window: &mut [u8; WINDOW_BYTES], stream: &[u8]) -> Result<Vec<u8>, Error> {
    let header: [u8; HEADER_SIZE] = stream
        .get(..HEADER_SIZE)
        .and_then(|bytes| bytes.try_into().ok())
        .ok_or(Error::MissingHeader {
            length: stream.len(),
        })?;
    let size = u32::from_be_bytes(header);
    let mut reader = BitReader::new(&stream[HEADER_SIZE..]);
    let mut tree = AdaptiveTree::new();

    // Every symbol consumes at least one bit and yields at most LOOK_AHEAD
    // bytes, which bounds what an honest stream can claim.
    let plausible = stream.len().saturating_mul(8 * LOOK_AHEAD);
    let mut out = Vec::with_capacity((size as usize).min(plausible));

    // `Decode` keeps writing the window after the last requested byte and
    // stops only between symbols, so `count` may pass `size`.
    let mut r = PREFILL;
    let mut count: u32 = 0;
    while count < size {
        let mut node = usize::from(tree.son[ROOT]);
        while node < TREE_SIZE {
            node = usize::from(tree.son[node + reader.bit(size, &out)?]);
        }
        let symbol = node - TREE_SIZE;
        tree.update(symbol);
        if symbol < 256 {
            let byte = symbol as u8;
            out.push(byte);
            window[r] = byte;
            r = (r + 1) & MASK;
            count += 1;
        } else {
            let start = (r + WINDOW_SIZE - usize::from(reader.position(size, &out)?) - 1) & MASK;
            let length = symbol - 255 + MATCH_THRESHOLD;
            for k in 0..length {
                let byte = window[(start + k) & MASK];
                if count < size {
                    out.push(byte);
                }
                window[r] = byte;
                r = (r + 1) & MASK;
                count = count.wrapping_add(1);
            }
        }
    }
    Ok(out)
}

/// `getbuf`/`getlen`/`codePtr`: up to sixteen buffered bits, most
/// significant first.
struct BitReader<'a> {
    data: &'a [u8],
    next: usize,
    buffer: u16,
    length: u8,
}

impl<'a> BitReader<'a> {
    fn new(data: &'a [u8]) -> Self {
        Self {
            data,
            next: 0,
            buffer: 0,
            length: 0,
        }
    }

    /// Adds one byte below the buffered bits.
    fn refill(&mut self, declared: u32, out: &[u8]) -> Result<(), Error> {
        let byte = match self.data.get(self.next) {
            Some(&byte) => byte,
            None if self.next < self.data.len() + READ_AHEAD => 0,
            None => {
                return Err(Error::Truncated {
                    declared,
                    decoded: out.len(),
                })
            }
        };
        self.next += 1;
        self.buffer |= u16::from(byte) << (8 - self.length);
        self.length += 8;
        Ok(())
    }

    /// `GetBit`.
    fn bit(&mut self, declared: u32, out: &[u8]) -> Result<usize, Error> {
        while self.length <= 8 {
            self.refill(declared, out)?;
        }
        let bit = usize::from(self.buffer >> 15);
        self.buffer <<= 1;
        self.length -= 1;
        Ok(bit)
    }

    /// `DecodePosition`: the upper six bits come from the first eight code
    /// bits through `d_code`/`d_len`, the lower six bits follow.
    fn position(&mut self, declared: u32, out: &[u8]) -> Result<u16, Error> {
        while self.length <= 8 {
            self.refill(declared, out)?;
        }
        let first = usize::from(self.buffer >> 8);
        self.buffer <<= 8;
        self.length -= 8;

        let upper = u16::from(D_CODE[first]) << 6;
        let mut bits = first;
        for _ in 2..D_LEN[first] {
            bits = (bits << 1) + self.bit(declared, out)?;
        }
        Ok(upper | (bits & 0x3f) as u16)
    }
}
