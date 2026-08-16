/* Title:		Minimal C Minesweeper
 * Description:	A version of the Minesweeper game for the terminal written in C.
 * Keywords:	[#c, #minesweeper]
 * Author:		"[Jon Ruttan](jonruttan@gmail.com)"
 * Date:		2025-06-07
 * Revision:	2 (2026-08-15)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
 * so is inert to every cell_* operation. For any board narrower than 16 that
 * dead region also forms the sentinel border, and box() leans on it. At the
 * full 16 columns no such border exists, so box() masks the x edges directly.
 */
#define SHIFT	4
#define STRIDE	16
#define MASK	0x0f

typedef unsigned char uint8;
typedef unsigned short uint16;

char *chars = ".......... 12345678*XXXXXXXXXX?%";

uint8 board[256];
uint8 stack[256], *stack_p = stack;

uint8 width, height, mines;

/* A full 16x16 board has 256 playable cells, one more than a uint8 holds. */
uint16 score;

uint8 cell_inc(uint8 i, uint8 c)
{
	if (c < MINE) {
		c++;
	}

	return c;
}

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

uint8 cell_reveal(uint8 i, uint8 c)
{
	if (c < VISIBLE) {
		c += VISIBLE;
	}

	return c;
}

/* Apply fn to the three cells at j, j+1, j+2, skipping either end when it
 * would wrap around a row edge. j is a uint8, so it wraps within the board
 * rather than running off it.
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

uint8 xytoi(uint8 x, uint8 y)
{
	return (y << SHIFT) | x;
}

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

struct level {
	char *name;
	uint8 width, height, mines;
};

/* The classic expert board is 30x16, which needs more than the 16 columns a
 * uint8 index affords, so expert is squared off at 16x16 with the same 99.
 */
static struct level levels[] = {
	{ "classic",		10, 10, 10 },
	{ "beginner",		 9,  9, 10 },
	{ "intermediate",	16, 16, 40 },
	{ "expert",			16, 16, 99 },
	{ NULL,				 0,  0,  0 }
};

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
