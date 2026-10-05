//! The command-line front end.

use std::fs;
use std::path::PathBuf;
use std::process::Command;

fn scratch(name: &str) -> PathBuf {
    let dir = std::env::temp_dir().join(format!("homm1-lzhuf-cli-{}-{name}", std::process::id()));
    let _ = fs::remove_dir_all(&dir);
    fs::create_dir_all(&dir).unwrap();
    dir
}

fn run(args: &[&str]) -> std::process::Output {
    Command::new(env!("CARGO_BIN_EXE_homm1-lzhuf"))
        .args(args)
        .output()
        .unwrap()
}

#[test]
fn compress_and_decompress_files() {
    let dir = scratch("files");
    let input = dir.join("input");
    let stream = dir.join("input.lz");
    let output = dir.join("output");
    fs::write(&input, b"   abcabcabc and some text   ").unwrap();
    let path = |p: &PathBuf| p.to_str().unwrap().to_owned();

    assert!(run(&["compress", &path(&input), &path(&stream)])
        .status
        .success());
    assert!(run(&["decompress", &path(&stream), &path(&output)])
        .status
        .success());
    assert_eq!(fs::read(&output).unwrap(), fs::read(&input).unwrap());

    assert!(run(&[
        "decompress",
        "--fresh-window",
        &path(&stream),
        &path(&output)
    ])
    .status
    .success());
    assert_eq!(&fs::read(&output).unwrap()[..3], &[0, 0, 0]);

    let info = run(&["info", &path(&stream)]);
    assert!(String::from_utf8(info.stdout)
        .unwrap()
        .contains("declared length: 29"));
    let _ = fs::remove_dir_all(&dir);
}

#[test]
fn replay_runs_an_oracle_script() {
    let dir = scratch("replay");
    fs::write(dir.join("a"), b"hello hello hello").unwrap();
    fs::write(dir.join("seed.window"), vec![7u8; 100]).unwrap();
    fs::write(
        dir.join("script"),
        "# comment\nreset\nwindow seed.window\nencode a a.lz\ndump-window w\n",
    )
    .unwrap();
    let out = dir.join("out");
    let status = run(&[
        "replay",
        dir.join("script").to_str().unwrap(),
        out.to_str().unwrap(),
    ]);
    assert!(status.status.success(), "{status:?}");
    let window = fs::read(out.join("w")).unwrap();
    assert_eq!(window.len(), homm1_lzhuf::WINDOW_BYTES);
    assert_eq!(window[homm1_lzhuf::PREFILL - 1], b' ');
    assert_eq!(
        homm1_lzhuf::decompress(&fs::read(out.join("a.lz")).unwrap()).unwrap(),
        b"hello hello hello"
    );
    let _ = fs::remove_dir_all(&dir);
}

#[test]
fn bad_arguments_fail() {
    assert!(!run(&["unpack"]).status.success());
    assert!(
        !run(&["decompress", "/nonexistent/input", "/nonexistent/output"])
            .status
            .success()
    );
}
