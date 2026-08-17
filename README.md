# Micro Minesweeper in C

A minimalist version of the Minesweeper game in C.

Features:

- Tiny memory footprint -- 516 bytes of game state, nothing allocated.
- No multiplication/division, and nothing wider than a byte.
- Four levels, up to 16x16.
- Unit tests.

## Getting Started

The tests live in a submodule, so clone recursively:

```sh
git clone --recurse-submodules https://github.com/jonruttan/microminesweeper.git
```

Compile with:

```sh
make
```

Run with:

```sh
./minesweeper
```

Or pick a level:

| Level | Board | Mines |
| --- | --- | --- |
| `classic` (default) | 10x10 | 10 |
| `beginner` | 9x9 | 10 |
| `intermediate` | 16x16 | 40 |
| `expert` | 16x16 | 99 |

```sh
./minesweeper intermediate
```

Traditional expert is 30x16, but thirty columns will not fit a `uint8` index,
so it is squared off at 16x16 with the same 99 mines.

The game will draw the board, and wait for input:

```
Score: 90, Mines: 10, Move: 0
  0 1 2 3 4 5 6 7 8 9 
0 . . . . . . . . . . 
1 . . . . . . . . . . 
2 . . . . . . . . . . 
3 . . . . . . . . . . 
4 . . . . . . . . . . 
5 . . . . . . . . . . 
6 . . . . . . . . . . 
7 . . . . . . . . . . 
8 . . . . . . . . . . 
9 . . . . . . . . . . 
```

Enter the X and Y coordinates followed by an action, 0 for probe, 1 for flag.
Flagging toggles, so the same command clears a flag you've changed your mind
about.
Coordinates are hexadecimal, matching the labels, so a 16x16 board runs to
`f f`:

```
0 0 0
Score: 87, Mines: 10, Move: 1
  0 1 2 3 4 5 6 7 8 9 
0 1 1 . . . . . . . . 
1 1 . . . . . . . . . 
2 . . . . . . . . . . 
3 . . . . . . . . . . 
4 . . . . . . . . . . 
5 . . . . . . . . . . 
6 . . . . . . . . . . 
7 . . . . . . . . . . 
8 . . . . . . . . . . 
9 . . . . . . . . . . 
2 0 1
Score: 87, Mines: 10, Move: 2
  0 1 2 3 4 5 6 7 8 9 
0 1 1 X . . . . . . . 
1 1 . . . . . . . . . 
2 . . . . . . . . . . 
3 . . . . . . . . . . 
4 . . . . . . . . . . 
5 . . . . . . . . . . 
6 . . . . . . . . . . 
7 . . . . . . . . . . 
8 . . . . . . . . . . 
9 . . . . . . . . . . 
```

`Score` counts the safe cells still covered, so it starts at cells minus mines
and each one you uncover brings it down. Flagging does not move it -- reaching
zero means every safe cell is uncovered, which wins.

## Unit Tests

Run the unit tests with:

```sh
sh ./test-runner/test-runner.sh tests
```
