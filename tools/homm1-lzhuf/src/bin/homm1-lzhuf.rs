//! File-level front end for the `homm1-lzhuf` library.

use std::fs;
use std::io::{self, Read, Write};
use std::path::{Path, PathBuf};
use std::process::ExitCode;

use homm1_lzhuf::{compress, declared_length, decompress, Codec, WINDOW_BYTES};

const USAGE: &str = "\
usage:
  homm1-lzhuf compress INPUT OUTPUT
  homm1-lzhuf decompress [--fresh-window] INPUT OUTPUT
  homm1-lzhuf info INPUT
  homm1-lzhuf replay SCRIPT OUTDIR

INPUT or OUTPUT may be `-` for standard input or output.

compress     encode as a freshly started game process (EncodeData)
decompress   decode with the encoder's space-filled window; --fresh-window
             decodes with the zeroed window of a fresh receiving process
info         print the declared uncompressed length of a stream
replay       run an oracle script against one persistent codec state; the
             retail oracle runs the same script through HEROES.EXE";

fn main() -> ExitCode {
    let args: Vec<String> = std::env::args().skip(1).collect();
    match run(&args) {
        Ok(()) => ExitCode::SUCCESS,
        Err(message) => {
            eprintln!("homm1-lzhuf: {message}");
            ExitCode::FAILURE
        }
    }
}

fn run(args: &[String]) -> Result<(), String> {
    let args: Vec<&str> = args.iter().map(String::as_str).collect();
    match args.as_slice() {
        ["compress", input, output] => {
            let stream = compress(&read(input)?).map_err(|e| e.to_string())?;
            write(output, &stream)
        }
        ["decompress", input, output] => {
            let data = decompress(&read(input)?).map_err(|e| e.to_string())?;
            write(output, &data)
        }
        ["decompress", "--fresh-window", input, output] => {
            let data = Codec::new()
                .decode(&read(input)?)
                .map_err(|e| e.to_string())?;
            write(output, &data)
        }
        ["info", input] => {
            let stream = read(input)?;
            let length = declared_length(&stream).map_err(|e| e.to_string())?;
            println!("declared length: {length}");
            println!("code bytes: {}", stream.len() - homm1_lzhuf::HEADER_SIZE);
            Ok(())
        }
        ["replay", script, outdir] => replay(Path::new(script), Path::new(outdir)),
        ["-h" | "--help"] => {
            println!("{USAGE}");
            Ok(())
        }
        _ => Err(format!("unrecognised arguments\n{USAGE}")),
    }
}

/// Runs an oracle script. One operation per line; `#` starts a comment.
/// Inputs are relative to the script's directory, outputs to `outdir`.
///
/// ```text
/// reset                    fresh process state (zeroed window)
/// window INPUT             copy INPUT over the start of the window
/// encode INPUT OUTPUT      EncodeData
/// decode INPUT OUTPUT      DecodeData
/// dump-window OUTPUT       write the whole window
/// ```
fn replay(script: &Path, outdir: &Path) -> Result<(), String> {
    let text = fs::read_to_string(script).map_err(|e| format!("{}: {e}", script.display()))?;
    let base = script.parent().unwrap_or(Path::new("."));
    let input = |name: &str| -> Result<Vec<u8>, String> {
        let path = base.join(name);
        fs::read(&path).map_err(|e| format!("{}: {e}", path.display()))
    };
    let output = |name: &str, bytes: &[u8]| -> Result<(), String> {
        let path: PathBuf = outdir.join(name);
        if let Some(parent) = path.parent() {
            fs::create_dir_all(parent).map_err(|e| format!("{}: {e}", parent.display()))?;
        }
        fs::write(&path, bytes).map_err(|e| format!("{}: {e}", path.display()))
    };

    let mut codec = Codec::new();
    for (number, line) in text.lines().enumerate() {
        let line = line.split('#').next().unwrap_or("").trim();
        let words: Vec<&str> = line.split_whitespace().collect();
        let fail = |e: homm1_lzhuf::Error| format!("{}:{}: {e}", script.display(), number + 1);
        match words.as_slice() {
            [] => {}
            ["reset"] => codec = Codec::new(),
            ["window", name] => {
                let bytes = input(name)?;
                if bytes.len() > WINDOW_BYTES {
                    return Err(format!("{name}: more than {WINDOW_BYTES} window bytes"));
                }
                let mut window = *codec.window();
                window[..bytes.len()].copy_from_slice(&bytes);
                codec = Codec::with_window(&window);
            }
            ["encode", source, target] => {
                output(target, &codec.encode(&input(source)?).map_err(fail)?)?
            }
            ["decode", source, target] => {
                output(target, &codec.decode(&input(source)?).map_err(fail)?)?
            }
            ["dump-window", target] => output(target, codec.window())?,
            _ => {
                return Err(format!(
                    "{}:{}: unknown operation {line:?}",
                    script.display(),
                    number + 1
                ))
            }
        }
    }
    Ok(())
}

fn read(path: &str) -> Result<Vec<u8>, String> {
    if path == "-" {
        let mut bytes = Vec::new();
        io::stdin()
            .read_to_end(&mut bytes)
            .map_err(|e| format!("stdin: {e}"))?;
        Ok(bytes)
    } else {
        fs::read(path).map_err(|e| format!("{path}: {e}"))
    }
}

fn write(path: &str, bytes: &[u8]) -> Result<(), String> {
    if path == "-" {
        io::stdout()
            .write_all(bytes)
            .map_err(|e| format!("stdout: {e}"))
    } else {
        fs::write(path, bytes).map_err(|e| format!("{path}: {e}"))
    }
}
