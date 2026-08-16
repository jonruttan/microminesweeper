# Expansion

Recipes for extending the game. Each notes what it costs against the design
constraints in [design.md](design.md) — the point of the program is what it
refuses to do, so it is worth knowing which additions are free and which spend
something.

## Add a level

**Cost: nothing.**

Add a row to the table in `minesweeper.c`. The first entry is the default.

```c
static struct level levels[] = {
	{ "classic",		10, 10, 10 },
	{ "beginner",		 9,  9, 10 },
	{ "intermediate",	16, 16, 40 },
	{ "expert",			16, 16, 99 },
	{ "tiny",			 5,  5,  3 },
	{ NULL,				 0,  0,  0 }
};
```

Width and height must be 1–16. Mines above the cell count are clamped by
`init()`, so a level cannot hang the placement loop.

Non-square boards work: `{ "wide", 16, 8, 20 }` is fine. Only the 16-column
ceiling is fixed.

## Add unflagging

**Cost: one comparison.**

There is deliberately no way to remove a flag. Adding one means reversing what
`mark()` does and giving `score` its point back:

```c
int unmark(uint8 i)
{
	if (board[i] < MARKED || board[i] == INVALID) {
		return 0;
	}

	board[i] -= MARKED;
	score++;

	return 0;
}
```

The `INVALID` test is load-bearing: 31 is above `MARKED`, so without it an
off-board cell would be decremented into a valid-looking state.

Then give it an action code in `main()`:

```c
if (m == 1) {
	mark(i);
} else if (m == 2) {
	unmark(i);
} else {
	/* probe */
}
```

Add a test asserting that flag-then-unflag returns both the cell and the score
to where they started.

## Change the win condition

**Cost: nothing, but it changes the scoring model.**

Today `score` counts cells not yet accounted for, and flagging a *non-mine*
cell still counts it — so a board can be won with incorrect flags. See
[Design](design.md#scoring).

To require that every safe cell is actually uncovered, stop counting flags and
seed the score with safe cells only. In `init()`, after mine placement:

```c
score = 0;

for (i = 0; i < 256; i++) {
	if (board[i] < MINE) {
		score++;
	}
}
```

Then drop the `score--` from `mark()`. Now only `cell_probe` decrements, and
zero means every safe cell is uncovered.

Note the loop above needs `i` wider than `uint8`, or a different termination
test — `i < 256` is always true for a byte.

This also changes what the displayed score means, so update `usage.md`.

## Use the spare glyph

**Cost: nothing. One slot only.**

Index 30 in `chars` is `?` and unused:

```c
char *chars = ".......... 12345678*XXXXXXXXXX?%";
/*              0         1         2         3  */
```

It sits between the flagged range (20–29) and `INVALID` (31), so a cell set to
30 is inert to every `cell_*` function — they all test `>= MARKED` or lower.
That makes it a natural "question mark" state: visible, untouched by the fill,
but distinguishable from a flag.

Anything more than one new state means restructuring the table, because the
0–31 range is otherwise full. That is the real budget: **five bits per cell**.

## Go beyond 16x16

**Cost: the byte index, and most of what follows from it.**

This is the expensive one. The 16-column ceiling is not arbitrary — it is what
makes a `uint8` address the whole board.

To go wider you need:

1. **16-bit indices.** `board`, `stack`, `xytoi()`, `box()`, `box_row()`,
   `probe()`, `mark()` and the `cell_*` callbacks all take or return `uint8`
   indices.
2. **A bigger `stack`.** At `uint16`, 256 entries becomes 512 bytes.
3. **Different mine seeding.** `rand() & 0xff` covers the address space exactly
   today. A larger board needs a wider mask, and if the size is not a power of
   two, rejection sampling.
4. **A new stride.** Keep it a power of two — `y << 5` for 32 columns — or you
   reintroduce the multiply the program exists to avoid.
5. **Re-examine `score`.** Already `uint16`; a 32x32 board is 1024 cells, still
   fine, but check it against whatever size you pick.

A 32x16 board is the cheapest step up: 512 cells, stride 32, `SHIFT` of 5,
`MASK` of `0x1f`. It gets you the traditional expert board without leaving
power-of-two strides.

If you only need *one* more column of flexibility and not more cells, consider
instead going back to a `width + 1` sentinel stride and capping at 15 columns —
that buys back the edge masking in `box_row()` at the cost of the 16th column.

## Add first-move protection

**Cost: one board rebuild, or a deferred seed.**

Probing a mine on move 0 currently loses. Standard Minesweeper guarantees the
first probe is safe.

The cheap version is to reseed until the first probe is clear, in `main()`
before the first `probe()`:

```c
while (board[i] == MINE) {
	init(l->width, l->height, l->mines);
}
```

Correct and trivial, but it is a rejection loop whose cost climbs with mine
density — on `expert` at 39% it will re-roll often.

The better version defers seeding: have `init()` lay out the board without
mines, and place them on the first probe, excluding the probed cell and its
neighbourhood. That needs `init()` split into a layout half and a seeding half,
with `main()` calling the second after reading the first move.

## Retarget the display

**Cost: nothing. It is already isolated.**

`display()` is the only function that calls `printf` for board output, and it
reads state through the same `chars` table everything else does. Replacing it
does not touch game logic.

The `fn` parameter is the hook for a filtered view — it is applied to each cell
before drawing, *without* writing back to `board`:

```c
display(NULL);          /* as it stands */
display(cell_reveal);   /* mines exposed, board unchanged */
```

Write your own filter with the same signature to render a hint mode, a solver
overlay, or a debug view, without the display mutating the game.

For a curses front end, keep `display()` as the model and let the front end
read `board`, `width` and `height` directly — they are globals, which is
inconvenient for testing but convenient here.

## Adding tests for any of this

Anything touching `box()`, indexing, or the edges needs a test that pins the
16-column case specifically. That is where the sentinel border does not exist
and a wrap is silent — see `test_no_edge_wrap` in `tests/minesweeper.spec.c`
for the shape of it. Run `make sanitize` afterwards.
