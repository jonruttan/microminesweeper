/**
 * @file minesweeper.c
 * @brief Minimal C Minesweeper -- a terminal Minesweeper in a single file.
 *
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2025 Jon Ruttan
 * @license MIT
 * @date 2025-06-07
 * @version 2
 *
 * The design constraint is that no multiplication or division appears
 * anywhere. Most of what follows is a consequence of working around that. See
 * `docs/design.md` for the full rationale.
 *
 * @keywords [\#c, \#minesweeper]
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * @name Cell states
 *
 * A cell's value is also its index into ::chars, so drawing is a lookup with
 * no branching. Values below ::MINE are adjacent-mine counts.
 *
 * @{
 */
#define MINE	9	/**< Hidden mine. */
#define VISIBLE	10	/**< Added to uncover a cell: 10..19. */
#define MARKED	20	/**< Added again to flag a cell: 20..29. */
#define INVALID	31	/**< Off-board filler; inert to every cell_* function. */
/** @} */

/**
 * @name Board geometry
 *
 * The board is a fixed 16x16 address space, indexed `i = (y << SHIFT) | x`.
 *
 * The stride is a constant power of two, so an index is a shift and an or --
 * never a multiply -- and a uint8 addresses all 256 cells exactly once.
 *
 * Cells outside the active width x height hold ::INVALID, which is >= ::MARKED
 * and so is inert to every cell_* operation. For any board narrower than 16
 * that dead region also forms the sentinel border, and box() leans on it. At
 * the full 16 columns no such border exists, so box() masks the x edges
 * directly.
 *
 * @{
 */
#define SHIFT	4	/**< Row shift; log2 of ::STRIDE. */
#define STRIDE	16	/**< Cells per row. */
#define MASK	0x0f	/**< Column mask, and the highest valid x or y. */
/** @} */

typedef unsigned char uint8;
typedef unsigned short uint16;

/**
 * @brief Cell state to display glyph.
 *
 * Indexed directly by a cell's value: 0-9 hidden, 10 uncovered blank, 11-18
 * uncovered counts, 19 an uncovered mine, 20-29 flagged, 31 off-board.
 */
char *chars = ".......... 12345678*XXXXXXXXXX?%";

/** @brief The board, one byte per cell, covering the whole 16x16 space. */
uint8 board[256];

/** @brief Explicit stack for the flood fill, so probe() need not recurse. */
uint8 stack[256], *stack_p = stack;

uint8 width;	/**< Active board width, 1..16. */
uint8 height;	/**< Active board height, 1..16. */
uint8 mines;	/**< Mines placed, after clamping to the cell count. */

/**
 * @brief Cells not yet accounted for; the game is won when it reaches zero.
 *
 * @note This is the one counter a uint8 cannot hold: a full 16x16 board has
 * 256 playable cells, one more than a byte, which would wrap to zero and win
 * the game before the first move.
 */
uint16 score;

/**
 * @brief Increment an adjacent-mine count. Callback for box().
 *
 * Used when seeding mines. Values at or above ::MINE -- other mines, and
 * ::INVALID filler -- are left alone.
 *
 * @param i Cell index. Unused; present to match the box() callback signature.
 * @param c Current cell value.
 * @return The new cell value.
 */
uint8 cell_inc(uint8 i, uint8 c)
{
	if (c < MINE) {
		c++;
	}

	return c;
}

/**
 * @brief Uncover a cell and enqueue it if it is blank. Callback for box().
 *
 * Decrements ::score for each cell it uncovers. A cell that turns out to be
 * blank is pushed onto ::stack so probe() will expand it in turn; a numbered
 * cell is not, which is what stops the fill at a number.
 *
 * @param i Cell index, pushed onto ::stack when the cell is blank.
 * @param c Current cell value.
 * @return The new cell value.
 */
uint8 cell_probe(uint8 i, uint8 c)
{
	if (c >= MINE) {
		return c;
	}

	score--;

	if ((c += VISIBLE) == VISIBLE) {
		*stack_p++ = i;
	}

	return c;
}

/**
 * @brief Uncover a cell for the end-of-game reveal. Callback for box().
 *
 * Display-only: unlike cell_probe() it touches neither ::score nor ::stack,
 * and it is applied by display() without writing back to ::board.
 *
 * @param i Cell index. Unused; present to match the box() callback signature.
 * @param c Current cell value.
 * @return The value to draw.
 */
uint8 cell_reveal(uint8 i, uint8 c)
{
	if (c < VISIBLE) {
		c += VISIBLE;
	}

	return c;
}

/**
 * @brief Apply fn to the three cells at j, j+1 and j+2.
 *
 * Skips either end when it would wrap around a row edge. @p j is a uint8, so
 * it wraps within ::board rather than running off it.
 *
 * @param j  Index of the leftmost cell of the run.
 * @param x  Column of the centre cell, used to detect the row edges.
 * @param fn Callback applied to each cell in range.
 */
static void box_row(uint8 j, uint8 x, uint8 (*fn)(uint8, uint8))
{
	uint8 k;

	for (k = 0; k < 3; k++, j++) {
		if ((k == 0 && x == 0) || (k == 2 && x == MASK)) {
			continue;
		}

		board[j] = fn(j, board[j]);
	}
}

/**
 * @brief Apply fn to the 3x3 neighbourhood around a cell, clipped to the board.
 *
 * The single neighbourhood primitive: seeding mines (cell_inc()), uncovering
 * (cell_probe()) and revealing (cell_reveal()) are the same traversal with a
 * different callback, so the edge clipping lives in exactly one place. The
 * centre cell is included.
 *
 * @param i  Index of the centre cell.
 * @param fn Callback applied to each cell in the neighbourhood.
 * @return Always 0.
 */
int box(uint8 i, uint8 (*fn)(uint8, uint8))
{
	uint8 x = i & MASK, y = i >> SHIFT;

	if (y) {
		box_row(i - STRIDE - 1, x, fn);
	}

	box_row(i - 1, x, fn);

	if (y < MASK) {
		box_row(i + STRIDE - 1, x, fn);
	}

	return 0;
}

/**
 * @brief Lay out a board and seed it with mines.
 *
 * Fills the whole 16x16 space: cells inside @p w x @p h start at 0 and count
 * toward ::score, the rest hold ::INVALID. Mine positions come from
 * `rand() & 0xff`, which covers the address space exactly with no modulo;
 * ::INVALID cells are rejected because they are not below ::MINE.
 *
 * @param w Board width, 1..16.
 * @param h Board height, 1..16.
 * @param m Mines to place, clamped to the number of playable cells.
 * @return Always 0.
 */
int init(uint8 w, uint8 h, uint8 m)
{
	uint8 x, y, i;

	width = w;
	height = h;
	score = 0;
	stack_p = stack;

	for (y = 0, i = 0; y <= MASK; y++) {
		for (x = 0; x <= MASK; x++, i++) {
			if (x < width && y < height) {
				board[i] = 0;
				score++;
			} else {
				board[i] = INVALID;
			}
		}
	}

	/* More mines than cells would spin the placement loop forever. */
	if (m > score) {
		m = score;
	}

	mines = m;

	while (m) {
		i = rand() & 0xff;

		if (board[i] < MINE) {
			board[i] = MINE;
			box(i, cell_inc);
			m--;
		}
	}

	return 0;
}

/**
 * @brief Convert a coordinate to a board index.
 *
 * A shift and an or, never a multiply, because ::STRIDE is a constant power
 * of two.
 *
 * @param x Column, 0..15.
 * @param y Row, 0..15.
 * @return The index into ::board.
 */
uint8 xytoi(uint8 x, uint8 y)
{
	return (y << SHIFT) | x;
}

/**
 * @brief Uncover a cell, flooding outward while cells come up blank.
 *
 * Iterative rather than recursive: cell_probe() pushes blank cells onto
 * ::stack and this drains it, so the fill's memory is bounded and visible. The
 * worst case measured is 141 of the 256 available entries, on a 16x16 board
 * with no mines.
 *
 * @param i Index of the cell to uncover.
 * @return 1 if the cell was a mine and the game is lost, otherwise 0.
 */
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

/**
 * @brief Flag a hidden cell, accounting for it against ::score.
 *
 * Cells at or above ::VISIBLE are already accounted for -- flagged, uncovered,
 * or ::INVALID filler -- and are left alone, so a repeated flag cannot drain
 * ::score toward a false win.
 *
 * @param i Index of the cell to flag.
 * @return Always 0.
 */
int mark(uint8 i)
{
	if (board[i] >= VISIBLE) {
		return 0;
	}

	while (board[i] < MARKED) {
		board[i] += VISIBLE;
	}

	score--;

	return 0;
}

/**
 * @brief Draw the active board, with hex row and column labels.
 *
 * @param fn Optional filter applied to each cell before it is drawn, without
 *           writing back to ::board. Pass cell_reveal() to expose the mines on
 *           a loss, or NULL to draw the board as it stands.
 */
void display(uint8 (*fn)(uint8, uint8))
{
	uint8 x, y, i, c;

	printf("  ");
	for (x = 0; x < width; x++) {
		printf("%hhx ", x);
	}

	for (y = 0; y < height; y++) {
		printf("\n%hhx ", y);

		for (x = 0; x < width; x++) {
			i = (y << SHIFT) | x;
			c = board[i];
			printf("%c ", chars[fn ? fn(i, c) : c]);
		}
	}

	printf("\n");
}

#ifndef TESTS
#include <string.h>

/** @brief A named difficulty preset. */
struct level {
	char *name;	/**< Name accepted on the command line. */
	uint8 width;	/**< Board width. */
	uint8 height;	/**< Board height. */
	uint8 mines;	/**< Mines to place. */
};

/**
 * @brief The selectable difficulty levels; the first is the default.
 *
 * @note The classic expert board is 30x16, which needs more than the 16
 * columns a uint8 index affords, so expert is squared off at 16x16 with the
 * same 99 mines.
 */
static struct level levels[] = {
	{ "classic",		10, 10, 10 },
	{ "beginner",		 9,  9, 10 },
	{ "intermediate",	16, 16, 40 },
	{ "expert",			16, 16, 99 },
	{ NULL,				 0,  0,  0 }
};

/**
 * @brief Look up a level by name.
 *
 * @param name Name to match.
 * @return The level, or NULL if no level has that name.
 */
static struct level *find_level(char *name)
{
	struct level *l;

	for (l = levels; l->name; l++) {
		if (!strcmp(l->name, name)) {
			return l;
		}
	}

	return NULL;
}

/**
 * @brief Print usage and the level table to stderr.
 *
 * @param prog Program name, as invoked.
 * @return 1, for returning straight out of main().
 */
static int usage(char *prog)
{
	struct level *l;

	fprintf(stderr, "usage: %s [level]\n\nlevels:\n", prog);

	for (l = levels; l->name; l++) {
		fprintf(stderr, "  %-14s %hhux%hhu, %hhu mines%s\n",
				l->name, l->width, l->height, l->mines,
				l == levels ? " (default)" : "");
	}

	return 1;
}

/**
 * @brief Play a game.
 *
 * Reads `x y action` per move, in hex to match the board labels, where action
 * is 0 to probe and 1 to flag.
 *
 * @param argc Argument count.
 * @param argv Arguments; argv[1] optionally names a level.
 * @return 0 on a completed game, 1 on an unknown level.
 */
int main(int argc, char *argv[])
{
	uint8 x, y, m, i;
	int count, moves = 0;
	struct level *l = levels;

	if (argc > 1 && (l = find_level(argv[1])) == NULL) {
		return usage(argv[0]);
	}

	srand(time(NULL));
	init(l->width, l->height, l->mines);

	while (1) {
		printf("Score: %u, Mines: %hhu, Move: %i\n", score, mines, moves);
		display(NULL);

		count = scanf("%hhx %hhx %hhu", &x, &y, &m);

		/* EOF, or input the format could not consume -- retrying would leave
		 * the offending bytes in the buffer and spin.
		 */
		if (count != 3) {
			printf("\n");
			break;
		}

		if (x >= width || y >= height) {
			printf("Off the board.\n");
			continue;
		}

		i = xytoi(x, y);

		if (m) {
			mark(i);
		} else {
			if (probe(i)) {
				printf("You lose, score: %u, moves: %i\n", score, moves);
				display(cell_reveal);
				break;
			}
		}

		if (score == 0) {
			printf("You win, %i moves.\n", moves);
			break;
		}

		moves++;
	}

	return 0;
}
#endif /* TESTS */
