# Micro Minesweeper in C

A minimalist version of the Minesweeper game in C.

Features:

- Tiny memory footprint.
- No multiplication/division.
- Unit tests.

## Getting Started

Compile with:

```sh
gcc minesweeper.c -o minesweeper
```

Run with:

```sh
./minesweeper
```

The game will draw the board, and wait for input:

```
Score: 100, Move: 0
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

Enter the X and Y coordinates followed by an action, 0 for probe, 1 for flag:

```
0 0 0
Score: 94, Move: 1
  0 1 2 3 4 5 6 7 8 9 
0     1 . . . . . . . 
1 1 1 1 . . . . . . . 
2 . . . . . . . . . . 
3 . . . . . . . . . . 
4 . . . . . . . . . . 
5 . . . . . . . . . . 
6 . . . . . . . . . . 
7 . . . . . . . . . . 
8 . . . . . . . . . . 
9 . . . . . . . . . . 
0 9 0
Score: 92, Move: 2
  0 1 2 3 4 5 6 7 8 9 
0     1 . . . . . . . 
1 1 1 1 . . . . . . . 
2 . . . . . . . . . . 
3 . . . . . . . . . . 
4 . . . . . . . . . . 
5 . . . . . . . . . . 
6 . . . . . . . . . . 
7 . . . . . . . . . . 
8 . 3 . . . . . . . . 
9 2 . . . . . . . . . 
9 0 0
Score: 27, Move: 3
  0 1 2 3 4 5 6 7 8 9 
0     1 . 1           
1 1 1 1 1 1           
2 . 1                 
3 . 1 1 1 1           
4 . . . . 2 1 1       
5 . . . . . . 1       
6 . . . . . 2 1       
7 . . . . . 1         
8 . 3 . 2 1 1         
9 2 . . 1             
3 0 1
Score: 26, Move: 4
  0 1 2 3 4 5 6 7 8 9 
0     1 X 1           
1 1 1 1 1 1           
2 . 1                 
3 . 1 1 1 1           
4 . . . . 2 1 1       
5 . . . . . . 1       
6 . . . . . 2 1       
7 . . . . . 1         
8 . 3 . 2 1 1         
9 2 . . 1             
```

## Unit Tests

Run the unit tests with:

```sh
sh ./test-runner/test-runner.sh tests
```
