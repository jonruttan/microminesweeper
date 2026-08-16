#include "test-runner.h"
#include "../minesweeper.c"

/* Count the mines adjacent to (x, y) from first principles, without going
 * through box(), so that box() is tested against something independent.
 */
static uint8 count_mines(uint8 x, uint8 y)
{
	int dx, dy, nx, ny;
	uint8 n = 0;

	for (dy = -1; dy <= 1; dy++) {
		for (dx = -1; dx <= 1; dx++) {
			nx = x + dx;
			ny = y + dy;

			if (nx < 0 || nx > MASK || ny < 0 || ny > MASK) {
				continue;
			}

			if (board[xytoi(nx, ny)] == MINE) {
				n++;
			}
		}
	}

	return n;
}

/* Returns int, not uint8: a full board is 256 cells, which a uint8 cannot
 * hold and would silently report as zero.
 */
static int count_cells(uint8 value)
{
	int i, n = 0;

	for (i = 0; i < 256; i++) {
		if (board[i] == value) {
			n++;
		}
	}

	return n;
}

static char *test_cell_inc(void)
{
	uint8 i = 0;

	_it_should("increment values less than MINE", MINE == cell_inc(i, MINE - 1));
	_it_should("not increment values equal to MINE", MINE == cell_inc(i, MINE));
	_it_should("not increment values greater than MINE", INVALID == cell_inc(i, INVALID));

	return NULL;
}

static char *test_cell_probe(void)
{
	uint8 i = 0;

	score = 255;
	stack_p = stack;

	_it_should("not increment values greater than or equal to MINE", MINE == cell_probe(i, MINE));
	_it_should("not have decremented the score", 255 == score);

	_it_should("increment values less than VISIBLE", VISIBLE == cell_probe(i, 0));
	_it_should("have incremented the stack pointer", stack + 1 == stack_p);
	_it_should("have decremented the score", 254 == score);

	_it_should("not increment values greater than or equal to VISIBLE", VISIBLE == cell_probe(i, VISIBLE));
	_it_should("not have incremented the stack pointer", stack + 1 == stack_p);
	_it_should("not have decremented the score", 254 == score);

	/* A numbered cell is still uncovered, so it still costs a point -- it just
	 * does not go on the stack, because the fill stops at a number.
	 */
	_it_should("increment values less than VISIBLE", VISIBLE + 1 == cell_probe(i, 1));
	_it_should("not have incremented the stack pointer", stack + 1 == stack_p);
	_it_should("have decremented the score", 253 == score);

	return NULL;
}

static char *test_cell_reveal(void)
{
	uint8 i = 0;

	score = 255;
	stack_p = stack;

	_it_should("increment values less than VISIBLE", VISIBLE == cell_reveal(i, 0));
	_it_should("increment values less than VISIBLE", VISIBLE + 1 == cell_reveal(i, 1));
	_it_should("uncover a mine", VISIBLE + MINE == cell_reveal(i, MINE));
	_it_should("not increment values greater than or equal to VISIBLE", VISIBLE == cell_reveal(i, VISIBLE));
	_it_should("not increment marked values", MARKED == cell_reveal(i, MARKED));

	/* cell_reveal is display-only: unlike cell_probe it touches neither the
	 * score nor the fill stack.
	 */
	_it_should("not have touched the score", 255 == score);
	_it_should("not have touched the stack pointer", stack == stack_p);

	return NULL;
}

uint8 test_fn_calls = 0;
uint8 test_fn_indicies[9];
uint8 test_fn(uint8 i, uint8 c)
{
	test_fn_indicies[test_fn_calls++] = i;

	return c;
}

static char *test_box(void)
{
	test_fn_calls = 0;
	_it_should("return zero", 0 == box(17, test_fn));
	_it_should("have called test_fn 9 times", 9 == test_fn_calls);
	_it_should("have visited the full 3x3 box", 0 == memcmp((uint8[]){ 0, 1, 2, 16, 17, 18, 32, 33, 34 }, test_fn_indicies, test_fn_calls));

	test_fn_calls = 0;
	_it_should("return zero", 0 == box(1, test_fn));
	_it_should("have called test_fn 6 times", 6 == test_fn_calls);
	_it_should("have clipped the row above", 0 == memcmp((uint8[]){ 0, 1, 2, 16, 17, 18 }, test_fn_indicies, test_fn_calls));

	test_fn_calls = 0;
	_it_should("return zero", 0 == box(0, test_fn));
	_it_should("have called test_fn 4 times", 4 == test_fn_calls);
	_it_should("have clipped the top left corner", 0 == memcmp((uint8[]){ 0, 1, 16, 17 }, test_fn_indicies, test_fn_calls));

	/* The right edge is the case with no sentinel column to stop it: index 15
	 * is (f, 0) and index 16 is (0, 1), adjacent in memory but not on the board.
	 */
	test_fn_calls = 0;
	_it_should("return zero", 0 == box(15, test_fn));
	_it_should("have called test_fn 4 times", 4 == test_fn_calls);
	_it_should("not have wrapped past the right edge", 0 == memcmp((uint8[]){ 14, 15, 30, 31 }, test_fn_indicies, test_fn_calls));

	test_fn_calls = 0;
	_it_should("return zero", 0 == box(240, test_fn));
	_it_should("have called test_fn 4 times", 4 == test_fn_calls);
	_it_should("have clipped the bottom left corner", 0 == memcmp((uint8[]){ 224, 225, 240, 241 }, test_fn_indicies, test_fn_calls));

	test_fn_calls = 0;
	_it_should("return zero", 0 == box(255, test_fn));
	_it_should("have called test_fn 4 times", 4 == test_fn_calls);
	_it_should("have clipped the bottom right corner", 0 == memcmp((uint8[]){ 238, 239, 254, 255 }, test_fn_indicies, test_fn_calls));

	return NULL;
}

static char *test_init(void)
{
	uint8 x, y;

	init(10, 10, 10);

	_it_should("seed the score with the playable cell count", 100 == score);
	_it_should("record the mine count", 10 == mines);
	_it_should("have placed exactly that many mines", 10 == count_cells(MINE));
	_it_should("have reset the fill stack", stack == stack_p);

	_it_should("mark the column past the width invalid", INVALID == board[xytoi(10, 0)]);
	_it_should("mark the row past the height invalid", INVALID == board[xytoi(0, 10)]);
	_it_should("mark the far corner invalid", INVALID == board[xytoi(MASK, MASK)]);
	_it_should("leave the last playable cell playable", INVALID != board[xytoi(9, 9)]);

	for (y = 0; y < 10; y++) {
		for (x = 0; x < 10; x++) {
			if (board[xytoi(x, y)] == MINE) {
				continue;
			}

			_it_should("give every cell its true adjacent mine count",
					count_mines(x, y) == board[xytoi(x, y)]);
		}
	}

	/* A full 16x16 is 256 cells -- one more than a uint8 score would hold. */
	init(16, 16, 40);
	_it_should("not overflow the score on a full board", 256 == score);
	_it_should("fill the whole address space", 0 == count_cells(INVALID));

	init(16, 16, 40);
	for (y = 0; y <= MASK; y++) {
		for (x = 0; x <= MASK; x++) {
			if (board[xytoi(x, y)] == MINE) {
				continue;
			}

			_it_should("count neighbours correctly with no sentinel border",
					count_mines(x, y) == board[xytoi(x, y)]);
		}
	}

	/* Asking for more mines than there are cells would spin forever. */
	init(9, 9, 200);
	_it_should("clamp the mine count to the board", 81 == mines);
	_it_should("have filled every cell with a mine", 81 == count_cells(MINE));

	return NULL;
}

static char *test_no_edge_wrap(void)
{
	init(16, 16, 0);

	_it_should("start from a clear board", 0 == count_cells(MINE));

	board[xytoi(MASK, 5)] = MINE;
	box(xytoi(MASK, 5), cell_inc);

	_it_should("increment the cell above", 1 == board[xytoi(MASK - 1, 4)]);
	_it_should("increment the cell left", 1 == board[xytoi(MASK - 1, 5)]);
	_it_should("increment the cell below", 1 == board[xytoi(MASK - 1, 6)]);

	/* These are the cells one past the right edge in memory. Reaching them
	 * would mean the fill had wrapped onto the following row.
	 */
	_it_should("not touch the next row", 0 == board[xytoi(0, 6)]);
	_it_should("not touch the row after", 0 == board[xytoi(0, 7)]);

	board[xytoi(0, 10)] = MINE;
	box(xytoi(0, 10), cell_inc);

	_it_should("increment the cell right", 1 == board[xytoi(1, 10)]);
	_it_should("not touch the previous row", 0 == board[xytoi(MASK, 9)]);
	_it_should("not touch the row before", 0 == board[xytoi(MASK, 10)]);

	return NULL;
}

static char *test_probe(void)
{
	/* No mines, so probing anywhere floods the entire board. */
	init(16, 16, 0);

	_it_should("survive the probe", 0 == probe(xytoi(8, 8)));
	_it_should("have uncovered every cell", 256 == count_cells(VISIBLE));
	_it_should("have run the score to zero", 0 == score);
	_it_should("have drained the fill stack", stack == stack_p);

	/* A cell walled off by numbers must not leak into the rest of the board. */
	init(16, 16, 0);
	board[xytoi(1, 0)] = MINE;
	box(xytoi(1, 0), cell_inc);

	_it_should("survive the probe", 0 == probe(xytoi(0, 0)));
	_it_should("uncover the probed cell", VISIBLE + 1 == board[xytoi(0, 0)]);
	_it_should("stop the fill at the number", 0 == board[xytoi(0, 2)]);

	init(10, 10, 0);
	board[xytoi(5, 5)] = MINE;

	_it_should("report stepping on a mine", 1 == probe(xytoi(5, 5)));

	return NULL;
}

static char *test_mark(void)
{
	score = 255;

	board[0] = 0;

	_it_should("return zero", 0 == mark(0));
	_it_should("mark a hidden empty cell", MARKED == board[0]);
	_it_should("have decremented the score", 254 == score);

	_it_should("return zero", 0 == mark(0));
	_it_should("not change an already marked cell", MARKED == board[0]);
	_it_should("not have decremented the score again", 254 == score);

	board[1] = MINE;

	_it_should("return zero", 0 == mark(1));
	_it_should("mark a hidden mine", MINE + MARKED == board[1]);
	_it_should("have decremented the score", 253 == score);

	board[2] = VISIBLE + 1;

	_it_should("return zero", 0 == mark(2));
	_it_should("not mark a revealed cell", VISIBLE + 1 == board[2]);
	_it_should("not have decremented the score", 253 == score);

	board[3] = INVALID;

	_it_should("return zero", 0 == mark(3));
	_it_should("not mark an invalid cell", INVALID == board[3]);
	_it_should("not have decremented the score", 253 == score);

	return NULL;
}

static char *test_xytoi(void)
{
	_it_should("return 0 for (0, 0)", 0 == xytoi(0, 0));
	_it_should("return 1 for (1, 0)", 1 == xytoi(1, 0));
	_it_should("return 16 for (0, 1)", 16 == xytoi(0, 1));
	_it_should("return 153 for (9, 9)", 153 == xytoi(9, 9));
	_it_should("return 255 for (f, f)", 255 == xytoi(MASK, MASK));

	return NULL;
}

static char *run_tests()
{
	_run_test(test_cell_inc);
	_run_test(test_cell_probe);
	_run_test(test_cell_reveal);
	_run_test(test_box);
	_run_test(test_init);
	_run_test(test_no_edge_wrap);
	_run_test(test_probe);
	_run_test(test_mark);
	_run_test(test_xytoi);

	return NULL;
}
