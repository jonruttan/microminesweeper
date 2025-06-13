/* Title:	Minimal C Minesweeper
 * Description:	A version of the Minesweeper game for the terminal written in C.
 * Keywords:	[#c, #minesweeper]
 * Author:	"[Jon Ruttan](jonruttan@gmail.com)"
 * Date:	2025-06-07
 * Revision:	1 (2025-06-7)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MINE	9
#define VISIBLE	10
#define MARKED	20
#define INVALID	31

typedef unsigned char uint8;

char *chars = ".......... 12345678*XXXXXXXXXX?%";

uint8 board[256];
uint8 stack[256], *stack_p = stack;

uint8 width, height, mines, score;

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

int box(uint8 i, uint8 (*fn)(uint8, uint8))
{
	uint8 j, k, *p;

	if (i > width) {
		for (j=i-width, p=board+j, k=3; k--; j--, p--) {
			*p = fn(j, *p);
		}
	}

	for (j=i-1, p=board+j, k=3; k--; j++, p++) {
		*p = fn(j, *p);
	}

	if (255 - i > width) {
		for (j=i+width, p=board+j, k=3; k--; j++, p++) {
			*p = fn(j, *p);
		}
	}

	return 0;
}

int init(uint8 w, uint8 h, uint8 m)
{
	uint8 i, j, k, n;

	width = w;
	height = h;
	mines = m;
	score = 0;

	for (i=255, j= 0, k=0; i;) {
		if (j-- && k <= height) {
			n = 0;
			score++;
		} else {
			n = INVALID;
		}

		board[255 - i--] = n;
		
		if (j > width) {
			j = width;
			k++;
		}	       
	}	

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

int probe(uint8 i)
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
	while (board[i] < MARKED) {
		board[i] += VISIBLE;
	}

	score--;

	return 0;
}

uint8 xytoi(uint8 x, uint8 y)
{
	for (; y--; ) {
		x += width + 1;
	}

	return ++x;
}

void display(uint8 (*fn)(uint8, uint8))
{
	uint8 x, y, i = 1, c;

	printf("  ");
	for (x=0; x < width; x++) {
		printf("%hhx ", x);
	}


	for (y=0; y < height; y++, i++) {
		printf("\n%hhx ", y);
		for (x=0; x < width; x++) {
			c = board[i++];
			printf("%c ", chars[fn ? fn(i, c) : c]);
		}
	}

	printf("\n");
}

#ifndef TESTS
int main(int argv, char *argc[], char *env[])
{
	uint8 x, y, m, i;
	int count, moves = 0;

	srand(time(NULL));
	init(10, 10, 10);

	while (1) {
		printf("Score: %i, Move: %i\n", score, moves);
		display(NULL);
		count = scanf("%hhx %hhx %hhu", &x, &y, &m);
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

