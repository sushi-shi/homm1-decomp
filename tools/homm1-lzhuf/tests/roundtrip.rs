//! Format and round-trip properties.

mod common;

use common::{geometric_bytes, lcg_bytes};
use homm1_lzhuf::{
    compress, declared_length, decompress, Codec, Error, HEADER_SIZE, PREFILL, WINDOW_BYTES,
    WINDOW_SIZE,
};

fn round_trip(input: &[u8]) -> Vec<u8> {
    let stream = compress(input).unwrap();
    assert_eq!(declared_length(&stream).unwrap() as usize, input.len());
    assert_eq!(&stream[..HEADER_SIZE], &(input.len() as u32).to_be_bytes());
    assert_eq!(decompress(&stream).unwrap(), input);
    stream
}

#[test]
fn empty_input_still_codes_the_wrapped_window() {
    // The unsigned look-ahead count wraps; 65 536 window positions are coded
    // after a zero length prefix, and decoding reads none of them.
    let stream = round_trip(b"");
    assert_eq!(&stream[..HEADER_SIZE], &[0, 0, 0, 0]);
    assert_eq!(stream.len(), 1449);
    assert_eq!(Codec::new().decode(&stream).unwrap(), b"");
    assert_eq!(decompress(&[0, 0, 0, 0]).unwrap(), b"");
}

#[test]
fn single_bytes() {
    for byte in 0..=255u8 {
        let stream = round_trip(&[byte]);
        assert!(stream.len() <= HEADER_SIZE + 2, "byte {byte:#04x}");
    }
}

#[test]
fn short_inputs_around_the_threshold_and_look_ahead() {
    for length in [2, 3, 4, 58, 59, 60, 61, 62, 119, 120, 121] {
        round_trip(&b"abcdefgh".repeat(16)[..length]);
        round_trip(&vec![b'z'; length]);
        round_trip(&lcg_bytes(length as u32, length, 0xff));
    }
}

#[test]
fn highly_repetitive_input_compresses() {
    let input = vec![b'a'; 200_000];
    let stream = round_trip(&input);
    // At most 60 bytes per match: about 22 bits per 60 input bytes.
    assert!(stream.len() < input.len() / 40);
}

#[test]
fn random_input_expands_slightly() {
    let input = lcg_bytes(0xdead, 50_000, 0xff);
    let stream = round_trip(&input);
    assert!(stream.len() > input.len());
    assert!(stream.len() < input.len() + input.len() / 50);
}

#[test]
fn inputs_beyond_the_window() {
    for length in [
        WINDOW_SIZE - 1,
        WINDOW_SIZE,
        WINDOW_SIZE + 1,
        3 * WINDOW_SIZE + 17,
    ] {
        round_trip(&lcg_bytes(7, length, 0x0f));
    }
    // Long matches across the window wrap, with skewed symbol frequencies
    // that force repeated tree rebuilds.
    round_trip(&geometric_bytes(11, 400_000));
    round_trip(&lcg_bytes(3, 300_000, 1));
}

#[test]
fn spaces_match_into_the_prefill() {
    let input = b"   three leading spaces, then text   ".to_vec();
    let stream = round_trip(&input);
    // A fresh receiver lacks the space prefill: each space the encoder
    // copied from there decodes as a zero byte, and nothing else changes.
    let fresh = Codec::new().decode(&stream).unwrap();
    assert_eq!(&fresh[..3], &[0, 0, 0]);
    assert_eq!(fresh.len(), input.len());
    for (decoded, original) in fresh.iter().zip(&input) {
        assert!(decoded == original || (*decoded == 0 && *original == b' '));
    }
}

#[test]
fn window_persists_across_calls() {
    let mut codec = Codec::new();
    let first = codec.encode(&lcg_bytes(1, 3000, 0xff)).unwrap();
    let window = *codec.window();
    assert!(window[..PREFILL].iter().any(|&byte| byte != b' '));
    // Decoding the stream rewrites the same window positions with the same
    // bytes, once the decoder starts from the encoder's prefill.
    let mut receiver = Codec::with_encoder_prefill();
    receiver.decode(&first).unwrap();
    assert_eq!(&receiver.window()[..WINDOW_SIZE], &window[..WINDOW_SIZE]);
    assert_eq!(codec.window().len(), WINDOW_BYTES);
}

#[test]
fn every_codec_state_produces_decodable_streams() {
    let mut codec = Codec::with_window(&lcg_bytes(99, WINDOW_BYTES, 0xff));
    for length in [0, 1, 5, 59, 60, 61, 1000] {
        let input = lcg_bytes(length as u32 + 1, length, 0x03);
        let stream = codec.encode(&input).unwrap();
        assert_eq!(decompress(&stream).unwrap(), input, "length {length}");
    }
}

#[test]
fn malformed_streams_are_errors() {
    assert_eq!(
        decompress(&[0, 0, 1]),
        Err(Error::MissingHeader { length: 3 })
    );
    let stream = compress(&lcg_bytes(5, 10_000, 0xff)).unwrap();
    let cut = &stream[..stream.len() - 40];
    assert!(matches!(
        decompress(cut),
        Err(Error::Truncated {
            declared: 10_000,
            ..
        })
    ));
    // A huge declared length with no code bytes fails without allocating it.
    assert!(matches!(
        decompress(&[0xff, 0xff, 0xff, 0xff]),
        Err(Error::Truncated { .. })
    ));
}
