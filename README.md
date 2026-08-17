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

Or give it a board, as width, height and mines:

```sh
./minesweeper 16 16 40
```

Width and height go up to 16, which is as far as a `uint8` index reaches. The
traditional 30x16 expert board will not fit.

Every game prints the seed it was dealt. Pass it back as a fourth argument to
play the same board again:

```sh
./minesweeper 10 10 10 31
```

The game will draw the board, and wait for input:

```
Seed: 31
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
Coordinates are hexadecimal, matching the labels, so a 16x16 board runs to
`f f`.

Each is a single digit, so the spaces are optional and the action can be left
off when probing -- `9 3 1`, `93 1` and `931` all flag the same square, and
`0 0 0`, `0 0` and `00` all probe the corner.

Probing an empty cell opens everything up to the surrounding numbers:

```
0 0 0
Score: 16, Mines: 10, Move: 1
  0 1 2 3 4 5 6 7 8 9 
0                     
1       1 1 1     1 1 
2   1 2 3 . 1     2 . 
3   1 . . 2 1     2 . 
4   1 2 2 1       1 1 
5                     
6     1 1 1           
7 1 1 1 . 3 2 1 1 1 1 
8 . . . . . . . . . . 
9 . . . . . . . . . . 
Score: 16, Mines: 10, Move: 2
  0 1 2 3 4 5 6 7 8 9 
0                     
1       1 1 1     1 1 
2   1 2 3 . 1     2 . 
3   1 . . 2 1     2 X 
4   1 2 2 1       1 1 
5                     
6     1 1 1           
7 1 1 1 . 3 2 1 1 1 1 
8 . . . . . . . . . . 
9 . . . . . . . . . . 
```

The `1` at `9 4` has one covered neighbour left, so `9 3` is a mine, and the
second move flags it. Flagging toggles -- the same command clears a flag you
have changed your mind about.

`Score` counts the safe cells still covered, so it starts at cells minus mines
and each one you uncover brings it down. Flagging does not move it -- reaching
zero means every safe cell is uncovered, which wins. A single probe on an open
board can take most of it at once.

`Mines` counts down as you flag and back up as you unflag, so it shows how many
are left to find. Flag more than there are and it goes negative.

## Unit Tests

Run the unit tests with:

```sh
sh ./test-runner/test-runner.sh tests
```
