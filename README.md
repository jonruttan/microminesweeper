# Micro Minesweeper in C

[![CI](https://github.com/jonruttan/microminesweeper/actions/workflows/ci.yml/badge.svg)](https://github.com/jonruttan/microminesweeper/actions/workflows/ci.yml)

A minimalist version of the Minesweeper game in C.

- Tiny memory footprint — 517 bytes of game state, nothing allocated at runtime.
- No multiplication or division anywhere in the program.
- Four difficulty levels, up to 16x16.
- Unit tests, sanitizers and CI.

```
Score: 200, Mines: 40, Move: 1
  0 1 2 3 4 5 6 7 8 9 a b c d e f
0 . . . . . . . . . . . . . . . .
1 . . . . . . . . . . . . . . . .
2 . . . . . . . . . . . . . . . .
3 . . . . . . . . . . . . . . . .
4 . . . . . . . . . . . . . . . .
5 . . . . . . . . . . . . . . . .
6 . . . . . . 1 1 1 2 . . . . . .
7 . . . . . . 1     1 . . . . . .
8 . . . . . 3 1     1 2 2 1 . . .
9 . . . . . 2             1 . . .
a . . . . . 4 1           1 . . .
b . . . . . . 2       1 2 4 . . .
c . . . . . . 3 1     1 . . . . .
d . . . . . . . 1   1 2 . . . . .
e . . . . . . 2 1   1 . . . . . .
f . . . . . . 1     1 . . . . . .
```

## Quick start

The test suite lives in a submodule, so clone recursively:

```sh
git clone --recurse-submodules https://github.com/jonruttan/microminesweeper.git
cd microminesweeper
make
./minesweeper
```

Enter an X coordinate, a Y coordinate, and an action — `0` to probe, `1` to
flag. Coordinates are hexadecimal, matching the row and column labels:

```
8 8 0
```

Pick a level with `./minesweeper intermediate`, or `make run LEVEL=intermediate`.

| Level | Board | Mines |
| --- | --- | --- |
| `classic` (default) | 10x10 | 10 |
| `beginner` | 9x9 | 10 |
| `intermediate` | 16x16 | 40 |
| `expert` | 16x16 | 99 |

## Documentation

| | |
| --- | --- |
| [Usage](docs/usage.md) | Playing the game: levels, coordinates, glyphs, scoring |
| [Design](docs/design.md) | How it works and why: addressing, cell states, the flood fill |
| [Development](docs/development.md) | Building, testing, CI, conventions, the test-runner pin |
| [Expansion](docs/expansion.md) | Recipes for extending it, and what each one costs |

An API reference is generated from the source with `make docs`.

## Testing

```sh
make test
make sanitize
```

`make test` first checks that the vendored test-runner matches the version the
[Makefile](Makefile) pins, so the two cannot drift apart unnoticed. See
[Development](docs/development.md#the-test-runner-pin).

## The short version of the design

The constraint is that no multiplication or division appears in the program.
Nearly everything else follows from that.

The board is a fixed 16x16 address space indexed `i = (y << 4) | x`. Because
the stride is a constant power of two, converting a coordinate is a shift and
an or, and a `uint8` addresses all 256 cells exactly once — which also means
`rand() & 0xff` picks a cell with no modulo.

Cells outside the active board hold a value high enough that every operation
already ignores it, so smaller boards get a sentinel border for free. A cell's
value is also its index into the glyph table, so drawing is a lookup with no
branching. The flood fill runs off an explicit 256-byte stack rather than the
call stack.

[Design](docs/design.md) has the whole of it, including the one counter that
did not fit in a byte.

## License

MIT. See [LICENSE](LICENSE).
