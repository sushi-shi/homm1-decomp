# Heroes of Might and Magic — Buka 2003 source, classic view

The generated C++ source of the 2003 Buka edition of Heroes of Might and Magic
(`HEROES.EXE`) and its scenario editor (`EDITOR.EXE`, `src/EDITOR/`) with their
text written out in Russian. Every text reference
of the source tree is replaced by the Russian string the retail program shows,
as readable UTF-8. Types, enums and code are those of `source-buka-2003`.

## Branches

```text
decomp-win95-1.0 ---> decomp-win95-1.1 ---> decomp-win95-1.2 ---> decomp-buka-2003
        |                                                                 |
        v                                                    +------------+------------+
source-win95-1.0                                             |                         |
                                                             v                         v
                                                     source-buka-2003         classic-buka-2003
                                                             |
                                                    +--------+--------+
                                                    |                 |
                                                    v                 v
                                                  port            source-te
                                                    |
                                                    v
                                                 port-te
```

| Branch | Purpose |
| --- | --- |
| [decomp-win95-1.0](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.0) | Win95 1.0 `HEROES.EXE` (Feb 1996) |
| [decomp-win95-1.1](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.1) | Win95 1.1 `HEROES.EXE` (May 1996) |
| [decomp-win95-1.2](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.2) | Win95 1.2 `HEROESW.EXE` (Aug 1997), VC4.1 |
| [decomp-buka-2003](https://github.com/sushi-shi/homm1-decomp/tree/decomp-buka-2003) | Buka 2003 game and editor, byte-identical |
| [source-win95-1.0](https://github.com/sushi-shi/homm1-decomp/tree/source-win95-1.0) | Clean source, Win95 1.0 |
| [source-buka-2003](https://github.com/sushi-shi/homm1-decomp/tree/source-buka-2003) | Clean source, Buka 2003 (ru/en) |
| [classic-buka-2003](https://github.com/sushi-shi/homm1-decomp/tree/classic-buka-2003) | Reading view, UTF-8 Russian |
| [port](https://github.com/sushi-shi/homm1-decomp/tree/port) | Native port: Linux, Windows, browser |
| [source-te](https://github.com/sushi-shi/homm1-decomp/tree/source-te) | Tournament Edition on the source |
| [port-te](https://github.com/sushi-shi/homm1-decomp/tree/port-te) | Tournament Edition on the port |

This branch is `classic-buka-2003`.

## Reading, not building

This view is for reading. Its strings are UTF-8, while the retail program
stores them as Windows-1251; the original compiler copies string bytes as
written, so the files would not compile to the retail text. To build, use
`source-buka-2003`, which keeps the text in a Russian and English catalog and
compiles either language with the original Visual C++ 6.0 SP5 toolchain.

## Regeneration

`decomp-buka-2003` generates this branch with `homm1 clean --variant classic`.
Make source changes there and regenerate; do not edit this branch by hand.

## License

Original project contributions are dedicated to the public domain under
[CC0 1.0](LICENSE), to the extent of the contributors' rights. This does not
grant rights to New World Computing's or Buka's game or to the Microsoft, RAD
Game Tools or other third-party material it uses. Game assets are not included.
