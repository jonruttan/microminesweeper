# Usage

## Building

```sh
make
```

Produces `./minesweeper`. Any C89 compiler will do; the Makefile defaults to
`cc` and honours `CC`:

```sh
make CC=clang
```

## Starting a game

```sh
./minesweeper              # classic, the default
./minesweeper intermediate
```

Or through the Makefile:

```sh
make run LEVEL=expert
```

An unknown level prints the table and exits non-zero:

```
usage: ./minesweeper [level]

levels:
  classic        10x10, 10 mines (default)
  beginner       9x9, 10 mines
  intermediate   16x16, 40 mines
  expert         16x16, 99 mines
```

## Levels

| Level | Board | Mines | Density |
| --- | --- | --- | --- |
| `classic` (default) | 10x10 | 10 | 10% |
| `beginner` | 9x9 | 10 | 12% |
| `intermediate` | 16x16 | 40 | 16% |
| `expert` | 16x16 | 99 | 39% |

The traditional expert board is 30x16. Thirty columns will not fit a `uint8`
index, so expert is squared off at 16x16 and keeps the 99 mines — which makes
it considerably denser than the original.

## Reading the board

```
Score: 256, Mines: 40, Move: 0
  0 1 2 3 4 5 6 7 8 9 a b c d e f
0 . . . . . . . . . . . . . . . .
1 . . . . . . . . . . . . . . . .
```

Row and column labels are **hexadecimal**, so a 16-wide board still labels
every column with a single character. Rows run `0`–`f` top to bottom, columns
`0`–`f` left to right.

| Glyph | Meaning |
| --- | --- |
| `.` | Hidden |
| (blank) | Uncovered, no adjacent mines |
| `1`–`8` | Uncovered, that many adjacent mines |
| `X` | Flagged |
| `*` | A mine, shown only after you lose |

The header line reports:

- **Score** — cells still unaccounted for. Counts down; zero wins.
- **Mines** — how many were placed. Fixed for the game.
- **Move** — moves made so far.

## Making a move

Enter three numbers: the X coordinate, the Y coordinate, and an action.

```
<x> <y> <action>
```

- **X and Y are hexadecimal**, matching the labels. The bottom-right corner of
  a 16x16 board is `f f`.
- **Action** is `0` to probe or `1` to flag.

Probe the top-left corner:

```
0 0 0
```

Flag the cell at column `c`, row `9`:

```
c 9 1
```

Coordinates off the active board are rejected with `Off the board.` and do not
count as a move.

## Probing

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

There is no first-move protection: probing a mine on move 0 loses.

## Flagging

Flagging marks a cell as a suspected mine and takes it out of play — a flagged
cell cannot be probed afterwards.

**There is no unflagging.** A flag is permanent for the rest of the game, so
place them only when certain. `../docs/expansion.md` covers adding one.

Note also that flagging a cell that is not a mine still counts it as accounted
for, so it is possible to reach zero — and win — with incorrect flags. This
follows from how scoring works; see [Design](design.md#scoring).

## Winning and losing

The game ends when:

- **Score reaches zero** — every cell is either uncovered or flagged.

  ```
  You win, 23 moves.
  ```

- **You probe a mine** — the board is redrawn with every mine shown as `*`.

  ```
  You lose, score: 250, moves: 3
  ```

- **Input ends** — EOF, or anything the input format cannot parse, exits
  quietly. This is what makes scripted games possible:

  ```sh
  printf '0 0 0\n8 8 0\n' | ./minesweeper intermediate
  ```
