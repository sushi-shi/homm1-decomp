//! Input generators shared with `homm1 verify lzhuf-oracle`
//! (`scripts/homm1/verify/lzhuf_oracle.py`), which records the retail outputs.

#![allow(dead_code)]

/// ANSI C `rand` stream: `(state >> 16) & mask` after each step.
pub fn lcg_bytes(seed: u32, count: usize, mask: u8) -> Vec<u8> {
    let mut state = seed;
    (0..count)
        .map(|_| {
            state = state.wrapping_mul(1_103_515_245).wrapping_add(12_345) & 0x7fff_ffff;
            (state >> 16) as u8 & mask
        })
        .collect()
}

/// Trailing zero count of each 15-bit LCG value: P(n) = 2^-(n+1).
pub fn geometric_bytes(seed: u32, count: usize) -> Vec<u8> {
    let mut state = seed;
    (0..count)
        .map(|_| {
            state = state.wrapping_mul(1_103_515_245).wrapping_add(12_345) & 0x7fff_ffff;
            ((state >> 16) | 0x8000).trailing_zeros() as u8
        })
        .collect()
}

pub fn fnv1a64(data: &[u8]) -> u64 {
    data.iter().fold(0xcbf2_9ce4_8422_2325, |hash, &byte| {
        (hash ^ u64::from(byte)).wrapping_mul(0x0000_0100_0000_01b3)
    })
}

pub fn hex(text: &str) -> Vec<u8> {
    (0..text.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&text[i..i + 2], 16).unwrap())
        .collect()
}

/// The generated oracle corpus by name (`generated()` in the oracle).
pub fn generated(name: &str) -> Vec<u8> {
    let repeat = |byte: u8, count: usize| vec![byte; count];
    match name {
        "empty" => Vec::new(),
        "byte-00" => vec![0x00],
        "byte-20" => vec![0x20],
        "byte-41" => vec![0x41],
        "byte-ff" => vec![0xff],
        "two" => b"AB".to_vec(),
        "three-spaces" => b"   ".to_vec(),
        "abc" => b"abc".to_vec(),
        "abcabcabc" => b"abcabcabc".to_vec(),
        "spaces-100" => repeat(b' ', 100),
        "leading-spaces" => b"    leading spaces reach into the prefill".repeat(3),
        "ramp" => (0..=255u8).collect::<Vec<_>>().repeat(4),
        "zeros-1000" => repeat(0, 1000),
        "a-59" => repeat(b'a', 59),
        "a-60" => repeat(b'a', 60),
        "a-61" => repeat(b'a', 61),
        "a-4095" => repeat(b'a', 4095),
        "a-4096" => repeat(b'a', 4096),
        "a-4097" => repeat(b'a', 4097),
        "random-17" => lcg_bytes(17, 17, 0xff),
        "random-1000" => lcg_bytes(1000, 1000, 0xff),
        "random-4096" => lcg_bytes(4096, 4096, 0xff),
        "zeros-100000" => repeat(0, 100_000),
        "spaces-5000" => repeat(b' ', 5000),
        "a-70000" => repeat(b'a', 70_000),
        "random-65536" => lcg_bytes(65536, 65536, 0xff),
        "random-300000" => lcg_bytes(300_000, 300_000, 0xff),
        "alphabet4-200000" => lcg_bytes(4, 200_000, 3),
        "geometric-150000" => geometric_bytes(2, 150_000),
        _ => panic!("unknown generated input {name}"),
    }
}
