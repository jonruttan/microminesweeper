/* Title:		Minimal C Minesweeper
 * Description:	A version of the Minesweeper game for the terminal written in C.
 * Keywords:	[#c, #minesweeper]
 * Author:		"[Jon Ruttan](jonruttan@gmail.com)"
 * Date:		2025-06-07
 * Revision:	1 (2025-06-07)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Cell states. A cell's value is also its index into chars[], so drawing is a
 * lookup with no branching and uncovering is an addition, not a state machine.
 * The ordering earns its keep: every "already dealt with" state sorts above
 * every "still hidden" one, so a single >= test makes a cell inert.
 *
 *   0-8    hidden, adjacent mine count    20-29  flagged
 *   9      hidden mine                    31     off the active board
 *   10-19  uncovered (19 is a hit mine)
 */
#define MINE	9
#define VISIBLE	10
#define MARKED	20
#define INVALID	31

/* The board is a fixed 16x16 address space, indexed i = (y << SHIFT) | x.
 *
 * The stride is a constant power of two, so an index is a shift and an or --
 * never a multiply -- and a uint8 addresses all 256 cells exactly once.
 *
 * Cells outside the active width x height hold INVALID, which is >= MARKED and
 * so is inert to every cell_* operation. That dead region doubles as the
 * sentinel border -- except at the full 16 columns, where none is left over;
 * see box_row().
 */
#define SHIFT	4
#define STRIDE	16
#define MASK	0x0f
#define ROW	0xf0

typedef unsigned char uint8;

/* Indexed by cell value; see the state table above. Slot 30 (?) is spare. */
char *chars = ".......... 12345678*XXXXXXXXXX?%";

uint8 board[256];
/* The flood fill's queue, so probe() need not recurse. */
uint8 stack[256], *stack_p = stack;

uint8 width, height, mines;

/* Safe cells left to uncover, not every unaccounted cell, so it tops out at
 * 256 mines and stays inside a uint8 even on a full board.
 */
uint8 score;

/* The three box() callbacks below each take a cell's index and value and
 * return its new value; only cell_probe has any use for the index.
 *
 * cell_inc raises an adjacent count when seeding a mine. Other mines, and
 * INVALID filler, are already at or above MINE and fall out untouched.
 */
uint8 cell_inc(uint8 i, uint8 c)
{
	if (c < MINE) {
		c++;
	}

	return c;
}

/* Uncover a cell, and queue it if it came up blank. Pushing exactly on the
 * 0 -> VISIBLE transition is what lets the fill terminate without a visited
 * set, since a cell can only make that transition once. A numbered cell is
 * uncovered but not queued, which is what stops the fill at a number.
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

/* Uncover for the end-of-game reveal. Display-only: unlike cell_probe it
 * touches neither the score nor the stack, and display() discards the result.
 */
uint8 cell_reveal(uint8 i, uint8 c)
{
	if (c < VISIBLE) {
		c += VISIBLE;
	}

	return c;
}

/* Apply fn to the three cells at j, j+1, j+2, skipping any that fell out of
 * the row. j is a uint8, so it wraps inside the board rather than running off
 * it -- and a cell that wrapped past a row edge lands in a different row,
 * which is exactly what the high nibble catches. That one test is the whole
 * 16-wide edge case; at any narrower width the INVALID border absorbs it.
 */
static void box_row(uint8 j, uint8 row, uint8 (*fn)(uint8, uint8))
{
	uint8 k;

	for (k = 0; k < 3; k++, j++) {
		if ((j & ROW) == row) {
			board[j] = fn(j, board[j]);
		}
	}
}

/* Apply fn to the 3x3 neighbourhood around a cell, the centre included.
 * Columns are clipped by the row mask above; rows need these guards instead,
 * because a row that ran off the top or bottom is still a valid row.
 */
int box(uint8 i, uint8 (*fn)(uint8, uint8))
{
	uint8 y = i >> SHIFT;

	if (y) {
		box_row(i - STRIDE - 1, (i - STRIDE) & ROW, fn);
	}

	box_row(i - 1, i & ROW, fn);

	if (y < MASK) {
		box_row(i + STRIDE - 1, (i + STRIDE) & ROW, fn);
	}

	return 0;
}

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

	/* score now holds the playable count modulo 256, so a zero here means the
	 * full 16x16 -- which a uint8 mine count cannot oversubscribe anyway. Every
	 * smaller board counted exactly, so clamp against it; more mines than cells
	 * would spin the placement loop forever.
	 */
	if (score && m > score) {
		m = score;
	}

	mines = m;

	/* Safe cells. Both terms are modulo 256, but their difference is not, so
	 * this lands on the true count without ever widening. Needs one mine on a
	 * full board, which every level places.
	 */
	score -= m;

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

/* A shift and an or, never a multiply -- the whole reason the stride is 16. */
uint8 xytoi(uint8 x, uint8 y)
{
	return (y << SHIFT) | x;
}

/* Uncover a cell, spreading while cells come up blank. Iterative: cell_probe
 * pushes and this drains, so the fill's memory is the stack array rather than
 * the call stack. Returns 1 if the cell was a mine.
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

/* Toggle the flag on a cell. A hidden cell is 0-9, so adding MARKED lands it
 * in the flagged band, and subtracting MARKED puts it back exactly as it was.
 * Uncovered cells and INVALID filler match neither test and are left alone --
 * INVALID being above the band is what keeps it out of the second one.
 */
int mark(uint8 i)
{
	if (board[i] < VISIBLE && mines) {
		board[i] += MARKED;
		mines--;
	} else if (board[i] >= MARKED && board[i] <= MARKED + MINE) {
		board[i] -= MARKED;
		mines++;
	}

	return 0;
}

/* Draw the active board with hex labels. fn filters each cell on its way out
 * without writing back, so passing cell_reveal exposes the mines on a loss
 * while leaving the board itself alone.
 */
void display(uint8 (*fn)(uint8, uint8))
{
	uint8 x, y, i, c;

	printf("  ");
	for (x=0; x < width; x++) {
		printf("%hhx ", x);
	}

	for (y=0; y < height; y++) {
		printf("\n%hhx ", y);
		for (x=0; x < width; x++) {
			i = (y << SHIFT) | x;
			c = board[i];
			printf("%c ", chars[fn ? fn(i, c) : c]);
		}
	}

	printf("\n");
}

#ifndef TESTS

int main(int argc, char *argv[])
{
	uint8 x, y, m, i, w = 10, h = 10, n = 10;
	unsigned seed = time(NULL);
	int count, moves = 0;

	if (argc >= 4) {
		w = atoi(argv[1]);
		h = atoi(argv[2]);
		n = atoi(argv[3]);
	}

	if (argc == 5) {
		seed = strtoul(argv[4], NULL, 0);
	}

	/* The board is a 16x16 address space, and a zero dimension leaves the
	 * mine placement loop hunting for a cell that does not exist.
	 */
	if ((argc != 1 && argc != 4 && argc != 5) || !w || w > STRIDE || !h || h > STRIDE) {
		fprintf(stderr, "usage: %s [width height mines [seed]]\n", argv[0]);

		return 1;
	}

	/* Printed so a board can be played again: pass it back as the fourth
	 * argument and the same mines land in the same places.
	 */
	srand(seed);
	printf("Seed: %u\n", seed);
	init(w, h, n);

	while (1) {
		printf("Score: %hhu, Mines: %hhu, Move: %i\n", score, mines, moves);
		display(NULL);
		/* One hex digit each, so 00 and 0 0 both read as (0, 0). */
		count = scanf("%1hhx%1hhx%*[ \t]", &x, &y);

		/* EOF, or input the format could not consume -- retrying would leave
		 * the offending bytes in the buffer and spin.
		 */
		if (count != 2) {
			printf("\n");
			break;
		}

		/* 0 or nothing probes, anything above marks -- newline and EOF sort
		 * below '1', so an omitted action needs no test of its own.
		 */
		m = getchar() >= '1';

		if (x >= width || y >= height) {
			printf("Off the board.\n");
			continue;
		}

		i = xytoi(x, y);

		if (m) {
			mark(i);
		} else {
			if (probe(i)) {
				printf("You lose, score: %hhu, moves: %i\n", score, moves);
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
