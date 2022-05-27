/*
 * A simple backtracking sudoku solver.  Accepts input with cells, dot (.)
 * to represent blank spaces and rows separated by newlines. Output format is
 * the same, only solved, so there will be no dots in it.
 *
 * Copyright (c) Mitchell Johnson (ehntoo@gmail.com), 2012
 * Modifications 2019 by Jeff Zarnett (jzarnett@uwaterloo.ca) for the purposes
 * of the ECE 459 assignment.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include "common.h"

/* Check the common header for the definition of puzzle */

// init shared var
FILE *inputfile;
FILE *outputfile;
int current_puzzle = 0;
int num_threads = 1;
int num_of_jobs = 81;
int empty_x_pos_1 = 0;
int empty_y_pos_1 = 0;
int empty_x_pos_2 = 0;
int empty_y_pos_2 = 0;

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;
pthread_mutex_t mutex3;
pthread_mutex_t mutex4;
pthread_mutex_t mutex5;

puzzle *output;

/* Check if current number is valid in this position;
 * returns 1 if yes, 0 if not */
int is_valid(int number, puzzle *p, int row, int column);

int solve(puzzle *p, int row, int column);

void write_to_file(puzzle *p, FILE *outputfile);

void* solve_new_puzzle();

void print_puzzle(puzzle *p);

void find_empty_pos (puzzle *p){
    int count = 2;
    for(int i = 0; i < 9; i++){
        for(int j = 0; j < 9; j++){
            if (count == 2){
                if (p->content[i][j] == 0) {
                    empty_x_pos_1 = i;
                    empty_y_pos_1 = j;
                    count --;
                }
            }
            else if (count == 1){
                if (p->content[i][j] == 0) {
                    empty_x_pos_2 = i;
                    empty_y_pos_2 = j;
                    count --;
                }
            } else {
                return;
            }
        }
    }
}

int main(int argc, char **argv) {
    puzzle *p;
    
    /* Parse arguments */
    int c;
    char *filename = NULL;
    while ((c = getopt(argc, argv, "t:i:")) != -1) {
        switch (c) {
            case 't':
                num_threads = strtoul(optarg, NULL, 10);
                if (num_threads == 0) {
                    printf("%s: option requires an argument > 0 -- 't'\n", argv[0]);
                    return EXIT_FAILURE;
                }
                break;
            case 'i':
                filename = optarg;
                break;
            default:
                return -1;
        }
    }

    // init
    output = malloc(sizeof(puzzle));
    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);
    pthread_mutex_init(&mutex3, NULL);
    pthread_mutex_init(&mutex4, NULL);
    pthread_mutex_init(&mutex5, NULL);

    /* Open Files */
    inputfile = fopen(filename, "r");
    if (inputfile == NULL) {
        printf("Unable to open input file.\n");
        return EXIT_FAILURE;
    }
    outputfile = fopen("output.txt", "w+");
    if (outputfile == NULL) {
        printf("Unable to open output file.\n");
        return EXIT_FAILURE;
    }

    /* Main loop - solve puzzle, write to file.
     * The read_next_puzzle function is defined in the common header */
    while ((p = read_next_puzzle(inputfile)) != NULL) {
        current_puzzle++;
        // printf("current_puzzle: %d\n", current_puzzle);

        find_empty_pos(p);

        pthread_t threads[num_threads];
        int thread_idx = 0;
        // each puzzle has 81 possibles => 81 jobs
        // Loop through 81 jobs
        for (int i = 1; i < 10; i++){
            if (is_valid(i, p, empty_x_pos_1, empty_y_pos_1) == 0){
                continue;
            }
            p->content[empty_x_pos_1][empty_y_pos_1] = i;
            for (int j = 1; j < 10; j++){
                if (is_valid(j, p, empty_x_pos_2, empty_y_pos_2) == 0){
                    continue;
                }
                p->content[empty_x_pos_2][empty_y_pos_2] = j;

                // the new puzzle is ready
                puzzle *input_puzzle = malloc(sizeof(puzzle));
                memcpy(input_puzzle, p, sizeof(puzzle));

                // printf("thread_idx: %d\n", thread_idx);
                // create pthread for each job
                pthread_create(&threads[thread_idx], NULL, solve_new_puzzle, input_puzzle);
                // printf("threads[thread_idx] %ld\n", threads[thread_idx]);

                // if number of active threads reaches num_threads, join...
                if (thread_idx == num_threads - 1){
                    // join threads
                    for (int i = 0; i < num_threads; i++) {
                        pthread_join(threads[i], NULL);
                        threads[i] = NULL;
                    }
                    // reset index after a full num_threads batch
                    thread_idx = 0;
                } else {
                    // keep track of reminder
                    thread_idx ++;
                }
                // current = p;
                // free(input_puzzle);
                p->content[empty_x_pos_2][empty_y_pos_2] = 0;
            }
            p->content[empty_x_pos_1][empty_y_pos_1] = 0;
        }

        // printf("reminder thread_idx: %d\n", thread_idx);
        // if there is reminder, need to join the reminder works
        for (int i = 0; i < thread_idx; i++) {
            pthread_join(threads[i], NULL);
        }

        // free(p);
        // free(current);
    }

    fclose( inputfile );
    fclose( outputfile );
    return 0;
}

void* solve_new_puzzle(puzzle *p){
    // need to parallel the solve part
    if (solve(p, 0, 0)){
        // printf("Solved\n");
        //write
        write_to_file(p, outputfile);
    } 
}

/*
 * A recursive function that does all the gruntwork in solving
 * the puzzle.
 */
int solve(puzzle *p, int row, int column) {
    int nextNumber = 1;
    /*
     * Have we advanced past the puzzle?  If so, hooray, all
     * previous cells have valid contents!  We're done!
     */
    if (9 == row) {
        return 1;
    }

    /*
     * Is this element already set?  If so, we don't want to
     * change it.
     */
    if (p->content[row][column]) {
        if (column == 8) {
            if (solve(p, row + 1, 0)) return 1;
        } else {
            if (solve(p, row, column + 1)) return 1;
        }
        return 0;
    }

    /*
     * Iterate through the possible numbers for this empty cell
     * and recurse for every valid one, to test if it's part
     * of the valid solution.
     */
    for (; nextNumber < 10; nextNumber++) {
        if (is_valid(nextNumber, p, row, column)) {
            p->content[row][column] = nextNumber;
            if (column == 8) {
                if (solve(p, row + 1, 0)) return 1;
            } else {
                if (solve(p, row, column + 1)) return 1;
            }
            p->content[row][column] = 0;
        }
    }
    return 0;
}

/*
 * Checks to see if a particular value is presently valid in a
 * given position.
 */
int is_valid(int number, puzzle *p, int row, int column) {
    int modRow = 3 * (row / 3);
    int modCol = 3 * (column / 3);
    int row1 = (row + 2) % 3;
    int row2 = (row + 4) % 3;
    int col1 = (column + 2) % 3;
    int col2 = (column + 4) % 3;

    /* Check for the value in the given row and column */
    for (int i = 0; i < 9; i++) {
        if (p->content[i][column] == number) return 0;
        if (p->content[row][i] == number) return 0;
    }

    /* Check the remaining four spaces in this sector */
    if (p->content[row1 + modRow][col1 + modCol] == number) return 0;
    if (p->content[row2 + modRow][col1 + modCol] == number) return 0;
    if (p->content[row1 + modRow][col2 + modCol] == number) return 0;
    if (p->content[row2 + modRow][col2 + modCol] == number) return 0;
    return 1;
}

/*
 * Convenience function to print out the puzzle.
 */
void write_to_file(puzzle *p, FILE *outputfile) {
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (8 == j) {
                fprintf(outputfile, "%d\n", p->content[i][j]);
            } else {
                fprintf(outputfile, "%d", p->content[i][j]);
            }
        }
    }
    fprintf(outputfile, "\n\n");
}

void print_puzzle(puzzle *p) {
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (8 == j) {
                printf("%d\n", p->content[i][j]);
            } else {
                printf("%d", p->content[i][j]);
            }
        }
    }
    printf("\n\n");
}
