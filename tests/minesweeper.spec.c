#include "test-runner.h"
#include "../minesweeper.c"

static char *test_cell_inc(void)
{
   uint8 i = 0;

   _it_should("increment values less than MINE", MINE == cell_inc(i, MINE - 1));
   _it_should("not increment values greater than or equal to MINE", MINE == cell_inc(i, MINE));

   return NULL;
}

static char *test_cell_probe(void)
{
   uint8 i = 0;

   score = 255;
   stack_p = stack;

   _it_should("not increment values greater than or equal to MINE", MINE == cell_probe(i, MINE));
   _it_should("increment values less than VISIBLE", VISIBLE == cell_probe(i, 0));
   _it_should("have incremented the stack pointer", stack + 1 == stack_p);
   _it_should("have decremented the score", 254 == score);

   _it_should("not increment values greater than or equal to VISIBLE", VISIBLE == cell_probe(i, VISIBLE));
   _it_should("not have incremented the stack pointer", stack + 1 == stack_p);
   _it_should("not have decremented the score", 254 == score);

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
   _it_should("not increment values greater than or equal to VISIBLE", VISIBLE == cell_reveal(i, VISIBLE));

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

   /* Index 15 is (f, 0) and index 16 is (0, 1): adjacent in memory, but not
    * on the board, and at 16 columns there is no border cell between them.
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
   return NULL;
}

static char *test_probe(void)
{
   return NULL;
}

static char *test_mark(void)
{
   score = 255;

   board[0] = 0;

   _it_should("return zero", 0 == mark(0));
   _it_should("mark a hidden empty cell", MARKED == board[0]);
   _it_should("not have touched the score", 255 == score);

   _it_should("return zero", 0 == mark(0));
   _it_should("not change an already marked cell", MARKED == board[0]);
   _it_should("still not have touched the score", 255 == score);

   board[1] = MINE;

   _it_should("return zero", 0 == mark(1));
   _it_should("mark a hidden mine", MINE + MARKED == board[1]);
   _it_should("not have touched the score", 255 == score);

   board[2] = VISIBLE + 1;

   _it_should("return zero", 0 == mark(2));
   _it_should("not mark a revealed cell", VISIBLE + 1 == board[2]);
   _it_should("not have touched the score", 255 == score);

   board[3] = INVALID;

   _it_should("return zero", 0 == mark(3));
   _it_should("not mark an invalid cell", INVALID == board[3]);
   _it_should("not have touched the score", 255 == score);

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
   _run_test(test_probe);
   _run_test(test_mark);
   _run_test(test_xytoi);

   return NULL;
}
