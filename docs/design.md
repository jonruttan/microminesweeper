# Design

## Goals

- **Small**: the whole game state is a fixed 517 bytes, known at compile time.
  Nothing is allocated at runtime.
- **No multiply, no divide**: neither operator appears in the program. On the
  small targets this style suits, both are library calls rather than
  instructions.
- **Single file**: `minesweeper.c` is the program. The test suite includes it
  directly rather than linking against it.

## Non-goals

- Boards larger than 16x16. See [Addressing](#addressing) for the ceiling and
  `expansion.md` for what lifting it costs.
- A curses or graphical front end.
- Configurable rules. The levels are a fixed table.

## Memory map

| Object | Bytes | Purpose |
| --- | --- | --- |
| `board` | 256 | One byte per cell, the whole 16x16 space |
| `stack` | 256 | Flood-fill queue |
| `width`, `height`, `mines` | 3 | Active geometry |
| `score` | 2 | Cells not yet accounted for |
| **Total** | **517** | |

Plus two pointers (`stack_p`, `chars`) and the 33-byte glyph literal.

`board` and `stack` are both sized 256 because a `uint8` index cannot address
more, so there is no arithmetic that could reach past either.

## Addressing

The board is a fixed 16x16 address space:

```c
i = (y << SHIFT) | x;   /* SHIFT is 4 */
x = i & MASK;           /* MASK is 0x0f */
y = i >> SHIFT;
```

Because the stride is a constant power of two, converting a coordinate to an
index is a shift and an or. A `uint8` then addresses all 256 cells exactly
once, which has three consequences worth naming:

- `rand() & 0xff` selects a uniformly random cell with no modulo and no
  rejection sampling beyond the "is this cell free" test that mine seeding
  needs anyway.
- Index arithmetic wraps within the array instead of running off it. `box()`
  computes `i - STRIDE - 1` for the row above without checking whether that
  underflows, because on a `uint8` it cannot leave the board.
- Every index is valid, so there is no such thing as an out-of-range cell —
  only a cell that is off the *active* board, which is a different question and
  is answered by `INVALID` below.

### Why not a sentinel column

The earlier design used a `width + 1` stride, where a single dead column served
as both the right edge of one row and the left edge of the next. That is a
tidier trick and needs no edge tests at all.

It cannot reach 16 columns. A 16x16 board with a sentinel column needs
`16 * 17 = 272` cells, past what a byte indexes. Keeping it would have meant
16-bit indices throughout, a 512-byte `stack`, and losing `rand() & 0xff`.
Trading it for two comparisons in `box()` was the cheaper side of that.

## Cell states

A cell's value *is* its index into the glyph table, so drawing is a lookup with
no branching:

```c
char *chars = ".......... 12345678*XXXXXXXXXX?%";
```

| Value | Meaning | Glyph |
| --- | --- | --- |
| 0–8 | Hidden, with adjacent mine count | `.` |
| 9 | Hidden mine | `.` |
| 10 | Uncovered, blank | (space) |
| 11–18 | Uncovered, count 1–8 | `1`–`8` |
| 19 | Uncovered mine, shown on a loss | `*` |
| 20–29 | Flagged | `X` |
| 31 | Off the active board | `%` |

Uncovering is `+= VISIBLE` and flagging is a second `+= VISIBLE`, so a state
transition is an addition rather than a branch. The ordering is deliberate:
every "already dealt with" state sorts above every "still hidden" state, which
is why a single `>=` test is enough to make a cell inert.

Index 30 (`?`) is unused and free for an expansion.

## INVALID as a border

Cells outside the active `width x height` hold `INVALID` (31). Since 31 is
above `MARKED`, every `cell_*` function already ignores it:

- `cell_inc` only increments below `MINE`
- `cell_probe` returns early at or above `MINE`
- `mark` returns early at or above `VISIBLE`

So a board narrower than 16 gets its border for free, with no explicit bounds
check anywhere in the hot path. `init()` paints the dead region once and the
rest of the program never thinks about it again.

### The one case the border does not cover

At exactly 16 columns there is no dead region left. Index 15 is `(f, 0)` and
index 16 is `(0, 1)` — adjacent in memory, not adjacent on the board. So
`box_row()` masks the horizontal edges itself:

```c
if ((k == 0 && x == 0) || (k == 2 && x == MASK)) {
	continue;
}
```

Two comparisons per row of three cells, and only on the edges. `box()` handles
the vertical edges separately, by testing `y` before it walks the row above or
below — those cannot be caught by a column mask.

This is the case `test_no_edge_wrap` exists to pin down.

## One neighbourhood primitive

`box()` walks the 3x3 neighbourhood around a cell and applies a function
pointer to each cell in it. Three operations share it:

| Callback | Used by | Effect |
| --- | --- | --- |
| `cell_inc` | `init()` | Raise adjacent counts around a new mine |
| `cell_probe` | `probe()` | Uncover, and enqueue if blank |
| `cell_reveal` | `display()` | Uncover for the end-of-game reveal |

The value of routing all three through one traversal is that the edge-clipping
logic exists in exactly one place. A bug in it is a bug everywhere at once,
which sounds bad but means it is worth testing hard, and `test_box` does.

The callbacks take the cell index as well as its value. Only `cell_probe`
uses it — to push onto the fill stack — but the shared signature is what lets
`box()` stay agnostic.

## The flood fill

`probe()` uncovers a cell and spreads outward while cells come up blank. The
fill is iterative:

```c
uint8 probe(uint8 i)
{
	*stack_p++ = i;

	while (stack_p > stack) {
		i = *--stack_p;

		if (board[i] == MINE) {
			return 1;
		}

		box(i, cell_probe);
	}

	return 0;
}
```

The enqueueing lives in `cell_probe`, not here: a cell is pushed exactly when
it transitions from hidden-blank to uncovered-blank, which is the same moment
it stops being a candidate for pushing again. That is what makes the fill
terminate without a visited set.

A numbered cell is uncovered but never pushed, which is what stops the fill at
a number.

### Bounds

Every cell is pushed at most once, so the fill does at most 257 pushes: 256
transitions plus the initial seed. Measured worst case over random start
points is **141** of 256 entries, on a 16x16 board with no mines — the
pathological input, since any mine breaks the region up. A 16x16 board with 40
mines peaks around 42, and the default 10x10 around 31.

The stack is sized 256 because that is what a `uint8` index affords; the
headroom is free.

## The one place a byte is not enough

`score` is a `uint16`. Everything else in the program is a `uint8`, and the
temptation is to make this one match.

A full 16x16 board has 256 playable cells. `init()` counts them, so on a `uint8`
the count wraps to zero — and `main()` checks `score == 0` to detect a win, so
the game would announce a victory before the first move. This is not a
theoretical overflow; it is the exact size of the largest board the addressing
allows.

## Scoring

`score` counts cells *not yet accounted for*. A cell is accounted for when it
is either uncovered or flagged, and each transition decrements once. Zero means
every cell has been dealt with, which is the win condition.

One consequence is worth stating plainly: flagging a cell that is *not* a mine
also decrements `score` and freezes the cell. A board can therefore be won with
incorrect flags. That follows from counting cells accounted for rather than
safe cells revealed. `expansion.md` covers changing it.

## Input

Coordinates are read in hex to match the board labels, which run `0`–`f` so a
16-wide board still labels every column with one character:

```c
count = scanf("%hhx %hhx %hhu", &x, &y, &m);
```

A return other than 3 means EOF or bytes the format could not consume. Both end
the game, because retrying would leave the offending bytes in the buffer and
spin forever.
