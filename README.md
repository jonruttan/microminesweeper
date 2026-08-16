# Micro Minesweeper in C

A minimalist version of the Minesweeper game in C.

Features:

- Tiny memory footprint — the board is a single 256-byte array.
- No multiplication or division anywhere.
- Four difficulty levels, up to 16x16.
- Unit tests.

## Getting Started

The test suite lives in a submodule, so clone recursively:

```sh
git clone --recurse-submodules https://github.com/jonruttan/microminesweeper.git
```

If you already cloned without it:

```sh
make submodule
```

Build:

```sh
make
```

Run:

```sh
./minesweeper
```

Or pick a level:

```sh
make run LEVEL=intermediate
```

## Levels

| Level | Board | Mines |
| --- | --- | --- |
| `classic` (default) | 10x10 | 10 |
| `beginner` | 9x9 | 10 |
| `intermediate` | 16x16 | 40 |
| `expert` | 16x16 | 99 |

The traditional expert board is 30x16. Thirty columns will not fit a `uint8`
index, so expert is squared off at 16x16 with the same 99 mines. See
[Addressing](#addressing) below.

## Playing

The game draws the board and waits for input:

```
Score: 256, Mines: 40, Move: 0
  0 1 2 3 4 5 6 7 8 9 a b c d e f
0 . . . . . . . . . . . . . . . .
1 . . . . . . . . . . . . . . . .
...
```

Enter an X coordinate, a Y coordinate, and an action — `0` to probe, `1` to
flag. Coordinates are hexadecimal, matching the row and column labels, so the
bottom-right corner of a 16x16 board is `f f`:

```
8 8 0
```

Probing an empty cell floods outward until it reaches numbers:

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

`Score` counts the cells still unaccounted for. Probing a safe cell or flagging
a mine each bring it down by one; reaching zero wins.

## Unit Tests

```sh
make test
```

Or under AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
make sanitize
```

`make test` first runs `make verify-runner`, which checks that the vendored
test-runner matches `RUNNER_VERSION` in the [Makefile](Makefile). The pin and
the submodule therefore cannot drift apart unnoticed — bumping one without the
other fails the build with a diagnostic saying so. CI runs the same check.

## Design Notes

The interesting constraint is the absence of multiplication and division. Most
of the design falls out of working around that.

### Addressing

The board is a fixed 16x16 address space:

```c
i = (y << SHIFT) | x;   /* SHIFT is 4 */
```

Because the stride is a constant power of two, converting a coordinate to an
index is a shift and an or rather than a multiply. A `uint8` then addresses all
256 cells exactly once, and `rand() & 0xff` picks a uniformly random cell with
no modulo.

The cost is the ceiling: 16 columns, 16 rows. A `width + 1` stride with a
sentinel column would be more flexible, but 16 wide needs `16 * 17 = 272`
cells, past what a byte can index.

### INVALID as a border

Cells outside the active `width x height` hold `INVALID` (31). Every `cell_*`
function already ignores values at or above `MARKED`, so those cells are inert
without a single explicit bounds check — a board narrower than 16 gets its
border for free.

At exactly 16 columns there is no dead region left to act as a border, and
index 15 (`f`, `0`) sits directly beside index 16 (`0`, `1`) in memory. So
`box()` masks the horizontal edges itself:

```c
if ((k == 0 && x == 0) || (k == 2 && x == MASK)) {
	continue;
}
```

### One neighbourhood primitive

`box()` walks the 3x3 neighbourhood around a cell and applies a function
pointer to each. Seeding mines (`cell_inc`), uncovering (`cell_probe`) and
revealing on a loss (`cell_reveal`) are all the same traversal with a different
callback, so the edge-clipping logic exists in exactly one place.

### State is the glyph index

A cell's value indexes the display table directly:

```c
char *chars = ".......... 12345678*XXXXXXXXXX?%";
```

`0`–`8` are hidden counts, `9` is a hidden mine, `+VISIBLE` (10) uncovers, and
`+MARKED` (20) flags. Drawing a cell is one array lookup with no branching, and
uncovering is an addition rather than a state machine.

### Flood fill without recursion

`cell_probe` both uncovers a cell and, when that cell turns out to be blank,
pushes it onto an explicit 256-byte stack. `probe()` drains that stack. The
fill is iterative, so its memory is bounded and visible rather than living on
the call stack. A full 16x16 fill on an empty board peaks at 134 entries.

### The one place a byte is not enough

`score` is a `uint16`. A full 16x16 board has 256 playable cells — one more
than a `uint8` holds, which would wrap the count to zero and win the game
before the first move.
