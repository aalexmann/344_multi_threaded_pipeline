#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h> 

/*

A program with a pipeline of 4 threads that interact with each other as producers 
   and consumers:

1. Input thread is the first thread in the pipeline. It holds every character from the 
    user input, up until the new line character and then puts the line into buffer_1, 
    which it shares with the next thread in the pipeline.
    This first thread will stop processing once it reads and whole input line that
    says only "STOP", immediately followed by a line separator(newline).

2. Line Separator Thread gets and reads lines from buffer_1, replacing every line
    separator ("\n") with a space. It will then forward the line to buffer_2.

3. Plus Sign Thread gets and reads lines from buffer_2, replacing every pair of plus 
    signs ("++") with a "^" symbol. It will then forward the line to buffer_3.

4. Output Thread will take lines from buffer_3, and formulate them to lines of exactly
    80 characters before writing them to standard output.

*/


// Size of the buffers
#define MAX_CHAR 1000
// Number of lines that will be produced. This number is less than the size of the buffer. Hence, we can model the buffer as being unbounded.
#define MAX_LINES 50

// Buffer 1, shared resource between input thread and line separator thread
int buffer_1[MAX_CHAR];
// Number of items in the buffer
int count_1 = 0;
// Index where the input thread will put the next item
int produced_idx_1 = 0;
// Index where the line separator thread will pick up the next item
int consumed_idx_1 = 0;
// Initialize the mutex for buffer 1
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 1
pthread_cond_t empty_1 = PTHREAD_COND_INITIALIZER;

// Buffer 2, shared resource between line separator thread and plus sign thread
int buffer_2[MAX_CHAR];
// Number of items in the buffer
int count_2 = 0;
// Index where the plus sign thread will put the next item
int produced_idx_2 = 0;
// Index where the plus sign thread will pick up the next item
int consumed_idx_2 = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t empty_2 = PTHREAD_COND_INITIALIZER;

// Buffer 3, shared resource between plus sign thread and output thread
int buffer_3[MAX_CHAR];
// Number of items in the buffer
int count_3 = 0;
// Index where the plus sign thread will put the next item
int produced_idx_3 = 0;
// Index where the plus sign thread will pick up the next item
int consumed_idx_3 = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t empty_3 = PTHREAD_COND_INITIALIZER;