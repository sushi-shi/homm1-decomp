# Rust tools

A dependency-free Cargo workspace. Build output goes to the ignored
`build/cargo/` (`tools/.cargo/config.toml`).

| Crate | Purpose |
| --- | --- |
| [`homm1-lzhuf`](homm1-lzhuf) | Byte-exact LZHUF codec used by the game's network save transfer, with a `homm1-lzhuf` CLI |

```sh
nix develop .#build
cargo test --offline --manifest-path tools/Cargo.toml
nix flake check          # cargo fmt --check, cargo check and cargo test
homm1 verify lzhuf-oracle --data /path/to/HEROES   # retail comparison
```

## homm1-lzhuf

A safe Rust reimplementation of the codec linked as `BASE/LZHUF`
(`vendor/lzhuf/encoder.cpp`) and `BASE/LZHUFDEC`
(`vendor/lzhuf/decoder/Decoder.asm`). The game calls it only from
`TransmitSaveGame` and the matching receive handler in `SOURCE/GAME.cpp`, to
compress `REMOTE.GAM` for a network opponent. No retail data file is
LZHUF-compressed.

```sh
homm1-lzhuf compress INPUT OUTPUT           # EncodeData, fresh process
homm1-lzhuf decompress INPUT OUTPUT         # DecodeData, encoder's window
homm1-lzhuf decompress --fresh-window IN OUT  # DecodeData, fresh receiver
homm1-lzhuf info STREAM                     # declared length
homm1-lzhuf replay SCRIPT OUTDIR            # oracle script, one codec state
```

`-` reads standard input or writes standard output. The library exposes
`compress`, `decompress` and `Codec`, which carries the game's persistent
window across calls.

### Format

| Property | Value |
| --- | --- |
| Header | Uncompressed length, big-endian `u32`. `EncodeData` returns the code length without these four bytes. |
| Code bits | Most significant bit first; the last byte is zero-padded. No end-of-stream symbol. |
| Window | 4096 bytes. Match lengths 3 to 60 (`THRESHOLD` 2, look-ahead 60). |
| Symbols | 256 literals plus 58 match lengths (314), coded with an adaptive Huffman tree of 627 nodes. The tree is rebuilt when the root frequency reaches `0x8000`. |
| Initial tree | Okumura's `StartHuff` state, copied from `initialSon`, `initialFrequency` and `initialParent` on every call. |
| Positions | Upper six bits through the fixed `positionCode`/`positionLength` prefix code (decoded through `d_code`/`d_len`), then six raw bits. |
| Match choice | The longest match; the nearest of equally long matches. |

The game's variant has these properties, all reproduced:

* **Shared window.** `text_buf` (4155 bytes: the window plus a 59-byte
  look-ahead mirror) is a global that both directions share and never clear.
  The encoder fills its first 4036 bytes with spaces, but its search trees
  also read the remaining bytes, which still hold earlier data. For short
  inputs those stale bytes decide between equally long matches, so the
  encoding depends on earlier calls. `compress` models a fresh process
  (zeroed window); `Codec` models any sequence of calls.
* **Decoder prefill.** The decoder fills nothing. A stream that copies from
  the encoder's space prefill (any run of spaces in the first 4 KiB) decodes
  correctly only if the receiver's window holds spaces there. `decompress`
  supplies the encoder's prefill; `Codec::new().decode` and
  `--fresh-window` reproduce a fresh receiving process, which turns those
  spaces into zero bytes.
* **Empty input.** The look-ahead count is an `unsigned short`. For empty
  input it wraps, and `EncodeData` codes 65 536 window positions after a
  zero length prefix (1449 bytes from a fresh process). The decoder reads
  none of them.
* **Read-ahead.** The bit reader refills whole bytes and may read past the
  stream's last byte. Those reads return zero here. A stream that needs more
  than two such bytes is reported as truncated; the game would read the
  following memory.
* **Code width.** `EncodeCharacter` assembles a code in 16 bits, and `PutCode`
  uses 32-bit shifts truncated to its 16-bit buffer. Deeper paths would lose
  bits; the port keeps the same arithmetic. No tested input reached a code
  longer than 15 bits.

### Verifying exactness

`homm1 verify lzhuf-oracle` compares the crate with the retail machine code:

1. `oracle/retail_oracle.c` and `oracle/image_stub.c` are compiled with the
   pinned VC6 toolchain under Wine. The stub is a `/FIXED` executable at
   `HEROES.EXE`'s base whose uninitialised data spans the retail image, so
   Wine maps nothing else there. The DLL then loads the hash-verified
   `HEROES.EXE` sections over it and calls the game's own `EncodeData` and
   `DecodeData`. `PollSound` is patched to `RET`; nothing else is replaced.
   The addresses come from the source claims through `homm1.model`.
2. The same scripts run through `homm1-lzhuf replay`. Every encoded stream,
   decoded output and window dump must be identical. A `reset` reloads the
   image, so each case starts from a fresh process state; sessions without a
   reset check the shared window.
3. The corpus covers empty and one-byte inputs, the match threshold and
   look-ahead boundaries, inputs longer than the window, runs, random and
   skewed data with repeated tree rebuilds, text, the 692 KB retail
   executable, and real game files from `--data` (or the folder
   `homm1 play` remembers): `REMOTE.GAM`, saved games, campaign and scenario
   maps. Retail streams are then decoded both with the encoder's prefill and
   with a fresh window.

Results go to `build/lzhuf-oracle/`. The last full run compared 56 inputs,
including 24 real files (2.9 MB): 123 encoder outputs and 168 decoder
outputs were identical, and every retail stream decoded to its input with
the prefill. Nine, including the shipped `REMOTE.GAM`, decode differently in
a fresh receiving process.

`tests/retail_vectors.rs` pins the retail results for the generated inputs
(short streams byte for byte, longer outputs by length and FNV-1a digest from
`build/lzhuf-oracle/vectors.tsv`), so `cargo test` checks exactness without
Wine or retail files. `tests/tables.rs` checks the tables against the
reconstruction's initializers in `vendor/lzhuf/encoder.cpp`.
