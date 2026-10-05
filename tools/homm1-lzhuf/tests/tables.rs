//! The crate's tables against the reconstruction's typed initializers in
//! `vendor/lzhuf/encoder.cpp`, which strict comparison proves against the
//! retail data.

use homm1_lzhuf::raw;

const RECONSTRUCTION: &str = include_str!(concat!(
    env!("CARGO_MANIFEST_DIR"),
    "/../../vendor/lzhuf/encoder.cpp"
));

/// The integers of the brace initializer that follows `name[`.
fn initializer(name: &str) -> Vec<i64> {
    let start = RECONSTRUCTION
        .find(&format!(" {name}["))
        .unwrap_or_else(|| panic!("{name} not found in encoder.cpp"));
    let body = &RECONSTRUCTION[start..];
    let open = body.find('{').unwrap();
    let close = body.find('}').unwrap();
    body[open + 1..close]
        .split(',')
        .map(str::trim)
        .filter(|item| !item.is_empty())
        .map(
            |item| match item.strip_prefix("0x").or(item.strip_prefix("0X")) {
                Some(hex) => i64::from_str_radix(hex, 16).unwrap(),
                None => item.parse().unwrap(),
            },
        )
        .collect()
}

fn widen<T: Copy + Into<i64>>(values: &[T]) -> Vec<i64> {
    values.iter().map(|&value| value.into()).collect()
}

#[test]
fn initial_huffman_tree_matches_reconstruction() {
    let (son, freq, parent) = raw::initial_tree();
    assert_eq!(widen(&son), initializer("initialSon"));
    assert_eq!(widen(&freq), initializer("initialFrequency"));
    assert_eq!(widen(&parent), initializer("initialParent"));
}

#[test]
fn position_tables_match_reconstruction() {
    assert_eq!(widen(&raw::POSITION_LENGTH), initializer("positionLength"));
    assert_eq!(widen(&raw::POSITION_CODE), initializer("positionCode"));
    assert_eq!(widen(&raw::D_CODE), initializer("d_code"));
    assert_eq!(widen(&raw::D_LEN), initializer("d_len"));
}

#[test]
fn decoder_tables_invert_encoder_tables() {
    for upper in 0..64 {
        let length = raw::POSITION_LENGTH[upper];
        let code = raw::POSITION_CODE[upper];
        // Every byte that starts with this prefix decodes to `upper`.
        for suffix in 0..(1u16 << (8 - length)) {
            let index = usize::from(code) | usize::from(suffix);
            assert_eq!(usize::from(raw::D_CODE[index]), upper);
            // Eight bits read plus `d_len - 2` more is the prefix plus six bits.
            assert_eq!(raw::D_LEN[index], length);
        }
    }
}
