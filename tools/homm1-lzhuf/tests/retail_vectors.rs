//! Golden vectors from the retail `HEROES.EXE`.
//!
//! `homm1 verify lzhuf-oracle` runs the game's own `EncodeData` and
//! `DecodeData` on the generated inputs in `common::generated` and writes
//! each output's length and FNV-1a-64 digest to
//! `build/lzhuf-oracle/vectors.tsv`. The tables below are that file. Short
//! streams are also pinned byte for byte.

mod common;

use common::{fnv1a64, generated, hex, lcg_bytes};
use homm1_lzhuf::{compress, decompress, Codec};

/// `EncodeData` output (length prefix included) from a fresh process.
const ENCODED: &[(&str, usize, u64)] = &[
    ("a-4095", 117, 0x828efc3beb7ca82f),
    ("a-4096", 117, 0x4f47c362cbe7dc4d),
    ("a-4097", 117, 0xfc9b604f31ae456c),
    ("a-59", 8, 0x40f3cc37cd0869fd),
    ("a-60", 8, 0xb2d98fca3cbf95ed),
    ("a-61", 8, 0x8d12cb1163ed9fe1),
    ("a-70000", 1504, 0xc6393a6aea226e4d),
    ("abc", 8, 0xaf2edfe81ca239d2),
    ("abcabcabc", 10, 0x3a3f5d366fdda64b),
    ("alphabet4-200000", 61597, 0x6c5904dfaa9f3d1a),
    ("byte-00", 6, 0xd01bcdfa25740324),
    ("byte-20", 6, 0xd0522dfa25a235b4),
    ("byte-41", 6, 0xd0880dfa25cf8ec4),
    ("byte-ff", 5, 0xe4b970d92529eb8b),
    ("empty", 1449, 0xe1ca087e108ae40a),
    ("geometric-150000", 50967, 0x1d839e45ce573812),
    ("leading-spaces", 48, 0xe739565db97056ad),
    ("ramp", 310, 0x3955f177bcf5afdc),
    ("random-1000", 1043, 0xa99b21695a403963),
    ("random-17", 23, 0xeeafe8cf4306f757),
    ("random-300000", 300304, 0x21a87fa0e95d55da),
    ("random-4096", 4191, 0xa1092886dfcce1ab),
    ("random-65536", 65695, 0xa8547345cb3475bf),
    ("spaces-100", 9, 0xd31b83573f3af0e5),
    ("spaces-5000", 137, 0x9331178518ba877f),
    ("three-spaces", 7, 0xdaebd1f90506ef70),
    ("two", 7, 0xef06ee03502eebd8),
    ("zeros-1000", 38, 0x022868274ac2f053),
    ("zeros-100000", 2129, 0xb73a5c1e55dcac28),
];

/// `DecodeData` of the retail stream by a fresh process (zeroed window).
const FRESH_DECODED: &[(&str, usize, u64)] = &[
    ("a-4095", 4095, 0xf7ae154fbb38dba6),
    ("a-4096", 4096, 0x15a9fd7b219d7325),
    ("a-4097", 4097, 0x6d4cfc3a1e8adc8c),
    ("a-59", 59, 0xd5ce4fbc143bc8ea),
    ("a-60", 60, 0x895a07966195c431),
    ("a-61", 61, 0xf9bf3487d17c93f0),
    ("a-70000", 70000, 0x35519c4a74a6b895),
    ("abc", 3, 0xe71fa2190541574b),
    ("abcabcabc", 9, 0xd328b56182a25b1b),
    ("alphabet4-200000", 200000, 0xad8da5fcb4285b87),
    ("byte-00", 1, 0xaf63bd4c8601b7df),
    ("byte-20", 1, 0xaf639d4c8601817f),
    ("byte-41", 1, 0xaf63fc4c860222ec),
    ("byte-ff", 1, 0xaf64724c8602eb6e),
    ("empty", 0, 0xcbf29ce484222325),
    ("geometric-150000", 150000, 0xbfa98cee9973bd93),
    ("leading-spaces", 123, 0x4719c73971e828a2),
    ("ramp", 1024, 0x1e5698f9d66e6f25),
    ("random-1000", 1000, 0xe6b0a8eac898be60),
    ("random-17", 17, 0x19a1c798788982c9),
    ("random-300000", 300000, 0x5e45ff3e545349bd),
    ("random-4096", 4096, 0x1469285e38b5075a),
    ("random-65536", 65536, 0x49b2a64ccc8b1168),
    ("spaces-100", 100, 0x1fc05eb337858375),
    ("spaces-5000", 5000, 0xa55ccd8b52a7bfc5),
    ("three-spaces", 3, 0xd94d12186c0f2fb7),
    ("two", 2, 0x09086407b5a0edaa),
    ("zeros-1000", 1000, 0x12633b178b17a745),
    ("zeros-100000", 100000, 0x61b94018df3e37a5),
];

/// The window (`text_buf`, 4155 bytes) after the fresh-process encode.
const WINDOW_AFTER_ENCODE: &[(&str, usize, u64)] = &[
    ("a-4095", 4155, 0x2fb00d2651ae763b),
    ("a-4096", 4155, 0x54147160d8fc38ea),
    ("a-4097", 4155, 0x54147160d8fc38ea),
    ("a-59", 4155, 0x771de2f169c0273a),
    ("a-60", 4155, 0x58e331fb606c2e8b),
    ("a-61", 4155, 0x30bde5d767ce6e03),
    ("a-70000", 4155, 0x54147160d8fc38ea),
    ("abc", 4155, 0x7ca31a0ede742e2b),
    ("abcabcabc", 4155, 0x1af70c26a580bb53),
    ("alphabet4-200000", 4155, 0x0a5cd0bc02acb9e2),
    ("byte-00", 4155, 0x598704b482736d17),
    ("byte-20", 4155, 0x21c7292672d429b7),
    ("byte-41", 4155, 0x1748bcf43134f9bc),
    ("byte-ff", 4155, 0x0f1d7e917e36e3ce),
    ("empty", 4155, 0x598704b482736d17),
    ("geometric-150000", 4155, 0xc94a10ed66cf655c),
    ("leading-spaces", 4155, 0x75089e2c338004c4),
    ("ramp", 4155, 0x08e3c2977d8e8ec4),
    ("random-1000", 4155, 0xdb5816ef57335583),
    ("random-17", 4155, 0x393a67244f014eb1),
    ("random-300000", 4155, 0x199066aa56661451),
    ("random-4096", 4155, 0x44eb1a379e6c1ce9),
    ("random-65536", 4155, 0x3385a76bec7664b4),
    ("spaces-100", 4155, 0x2dd1e630ac406057),
    ("spaces-5000", 4155, 0xc8fa44fa07d08137),
    ("three-spaces", 4155, 0x77c2f5c46327f637),
    ("two", 4155, 0x1010145a7e4d3bee),
    ("zeros-1000", 4155, 0x2dc30548966471d7),
    ("zeros-100000", 4155, 0x0c241538bf3005d7),
];

/// Short retail streams, byte for byte.
const STREAMS: &[(&str, &str)] = &[
    ("byte-00", "00000001c600"),
    ("byte-20", "00000001d600"),
    ("byte-41", "00000001e680"),
    ("byte-ff", "000000018b"),
    ("two", "00000002e6f380"),
    ("three-spaces", "000000038c0000"),
    ("abc", "00000003f6fbbde0"),
    ("abcabcabc", "00000009f6fbbdf1e020"),
    ("a-59", "0000003bf6e18000"),
    ("a-60", "0000003cf6e20000"),
    ("a-61", "0000003df6fb4000"),
    ("spaces-100", "00000064c51dd884c0"),
];

fn check(table: &[(&str, usize, u64)], what: &str, produce: impl Fn(&str) -> Vec<u8>) {
    for &(name, length, digest) in table {
        let output = produce(name);
        assert_eq!(
            (output.len(), fnv1a64(&output)),
            (length, digest),
            "{what} of {name} differs from retail"
        );
    }
}

#[test]
fn short_streams_match_retail_bytes() {
    for &(name, stream) in STREAMS {
        assert_eq!(compress(&generated(name)).unwrap(), hex(stream), "{name}");
    }
}

#[test]
fn encoding_matches_retail() {
    check(ENCODED, "encoding", |name| {
        compress(&generated(name)).unwrap()
    });
}

#[test]
fn window_after_encoding_matches_retail() {
    check(WINDOW_AFTER_ENCODE, "window", |name| {
        let mut codec = Codec::new();
        codec.encode(&generated(name)).unwrap();
        codec.window().to_vec()
    });
}

#[test]
fn fresh_receiver_decoding_matches_retail() {
    check(FRESH_DECODED, "fresh decoding", |name| {
        let stream = compress(&generated(name)).unwrap();
        Codec::new().decode(&stream).unwrap()
    });
}

#[test]
fn prefilled_decoding_inverts_retail_streams() {
    for &(name, _, _) in ENCODED {
        let input = generated(name);
        assert_eq!(
            decompress(&compress(&input).unwrap()).unwrap(),
            input,
            "{name}"
        );
    }
}

#[test]
fn fresh_receiver_loses_prefill_spaces() {
    // Retail: a stream that starts with spaces refers to the encoder's
    // space prefill, which a fresh receiving process does not have.
    let stream = hex("000000038c0000");
    assert_eq!(Codec::new().decode(&stream).unwrap(), [0, 0, 0]);
    assert_eq!(decompress(&stream).unwrap(), b"   ");
}

/// Seeded and back-to-back sessions (`_sessions()` in the oracle).
#[test]
fn sessions_match_retail() {
    let seed = lcg_bytes(4155, 4155, 0xff);
    let short_1 = b"hello".to_vec();
    let short_2 = b"abcabcab".to_vec();
    let short_3 = lcg_bytes(59, 59, 7);
    let medium = lcg_bytes(5000, 5000, 15);
    let digest = |bytes: &[u8]| (bytes.len(), fnv1a64(bytes));

    let mut codec = Codec::with_window(&seed);
    let seeded_1 = codec.encode(&short_1).unwrap();
    assert_eq!(seeded_1, hex("00000005fa7c7f187fb0"));
    assert_eq!(
        digest(codec.window()),
        (4155, 0x53251f5f836c8328),
        "session-seeded-1.window"
    );
    assert_eq!(
        digest(&codec.encode(&short_2).unwrap()),
        (10, 0x21cadae8190acb28),
        "session-seeded-2.lz"
    );
    assert_eq!(
        digest(&codec.encode(&short_3).unwrap()),
        (55, 0x2105e092afff9da1),
        "session-seeded-3.lz"
    );
    assert_eq!(
        digest(codec.window()),
        (4155, 0x8d722432c4efc63f),
        "session-seeded-3.window"
    );

    let mut codec = Codec::new();
    assert_eq!(
        digest(&codec.encode(&medium).unwrap()),
        (3052, 0xed1526877e37e082),
        "session-chain-1.lz"
    );
    assert_eq!(
        digest(&codec.encode(&short_2).unwrap()),
        (10, 0x21cadae8190acb28),
        "session-chain-2.lz"
    );
    assert_eq!(
        digest(&codec.encode(&short_1).unwrap()),
        (10, 0xdb80db4de667d438),
        "session-chain-3.lz"
    );
    assert_eq!(
        digest(codec.window()),
        (4155, 0x835dbc2d6745c31d),
        "session-chain.window"
    );

    // Stale look-ahead bytes past the end of a short input decide between
    // equally long real matches, so the encoding depends on the window.
    let stale = lcg_bytes(17511, 11, 3);
    let fresh = compress(&stale).unwrap();
    let seeded = Codec::with_window(&seed).encode(&stale).unwrap();
    assert_eq!(fresh, hex("0000000bc76331f8b1803c3c2038"));
    assert_eq!(seeded, hex("0000000bc76331f8b1803c3c2018"));
    assert_eq!(decompress(&fresh).unwrap(), stale);
    assert_eq!(decompress(&seeded).unwrap(), stale);
}
