#include "test-runner.h"
#include "../minesweeper.c"

static char *test_cell_inc(void)
{
   uint8 i = 0, c = MINE - 1;

   _it_should("increment values less than MINE", MINE == cell_inc(i, c));
   _it_should("not increment values greater than or equal to MINE", MINE == cell_inc(i, c));

   return NULL;
}

static char *test_cell_probe(void)
{
   uint8 i = 0;

   score = 255;

   _it_should("not increment values greater than or equal to MINE", MINE == cell_probe(i, MINE));
   _it_should("increment values less than VISIBLE", VISIBLE == cell_probe(i, 0));
   _it_should("have incremented the stack pointer", stack + 1 == stack_p);
   _it_should("have decremented the score", 254 == score);

   _it_should("not increment values greater than or equal to VISIBLE", VISIBLE == cell_probe(i, VISIBLE));
   _it_should("not have incremented the stack pointer", stack + 1 == stack_p);
   _it_should("not have decremented the score", 254 == score);

   _it_should("increment values less than VISIBLE", VISIBLE + 1 == cell_probe(i, 1));
   _it_should("not have incremented the stack pointer", stack + 1 == stack_p);
   _it_should("not have decremented the score", 254 == score);

   return NULL;
}

static char *test_cell_reveal(void)
{
   uint8 i = 0;

   score = 255;

   _it_should("increment values less than VISIBLE", VISIBLE == cell_reveal(i, 0));
   _it_should("have incremented the stack pointer", stack + 1 == stack_p);
   _it_should("have decremented the score", 254 == score);

   _it_should("not increment values greater than or equal to VISIBLE", VISIBLE == cell_reveal(i, VISIBLE));
   _it_should("not have incremented the stack pointer", stack + 1 == stack_p);
   _it_should("not have decremented the score", 254 == score);

   _it_should("increment values less than VISIBLE", VISIBLE + 1 == cell_reveal(i, 1));
   _it_should("not have incremented the stack pointer", stack + 1 == stack_p);
   _it_should("not have decremented the score", 254 == score);

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
   width = 9;

    _it_should("return zero", 0 == box(1, test_fn));
   _it_should("have called test_fn 6 times", 6 == test_fn_calls);
   _it_should("have called test_fn with proper indicies", 0 == memcmp((uint8[]){ 0, 1, 2, 10, 11, 12 }, test_fn_indicies, test_fn_calls));

   test_fn_calls = 0;

   _it_should("return zero", 0 == box(11, test_fn));
   _it_should("have called test_fn 9 times", 9 == test_fn_calls);
   _it_should("have called test_fn with proper indicies", 0 == memcmp((uint8[]){ 2, 1, 0, 10, 11, 12, 20, 21, 22 }, test_fn_indicies, test_fn_calls));

   test_fn_calls = 0;

   _it_should("return zero", 0 == box(254, test_fn));
   _it_should("have called test_fn 6 times", 6 == test_fn_calls);
   _it_should("have called test_fn with proper indicies", 0 == memcmp((uint8[]){ 245, 244, 243, 253, 254, 255 }, test_fn_indicies, test_fn_calls));

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

static char *test_xytoi(void)
{
   width = 9;

   _it_should("return 1 for (0, 0)", 1 == xytoi(0, 0));
   _it_should("return 100 for (9, 9)", 100 == xytoi(9, 9));

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
   _run_test(test_xytoi);

   return NULL;
}
