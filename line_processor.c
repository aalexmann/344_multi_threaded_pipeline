#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


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

// Global variables:
#define MAX_CHAR 1000
#define MAX_LINES 50

// Start and end flags:
int keep_processing = 1;
int terminate = 0;

//// Buffers:
// Flag:
int buff_1_closed = 0;
// Buffer 1, shared resource between input thread and line separator thread:
char buffer_1[50000];
// Number of items in the buffer:
int count_1 = 0;
// Index where the input thread will put the next item:
int produced_idx_1 = 0;
// Index where the line separator thread will pick up the next item:
int consumed_idx_1 = 0;
// Initialize the mutex for buffer 1:
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 1:
pthread_cond_t not_empty_1 = PTHREAD_COND_INITIALIZER;

// Flag:
int buff_2_closed = 0;
// Buffer 2, shared resource between line separator thread and plus sign thread:
char buffer_2[(50000)];
// Number of items in the buffer:
int count_2 = 0;
// Index where the plus sign thread will put the next item:
int produced_idx_2 = 0;
// Index where the plus sign thread will pick up the next item:
int consumed_idx_2 = 0;
// Initialize the mutex for buffer 2:
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2:
pthread_cond_t not_empty_2 = PTHREAD_COND_INITIALIZER;

// Flag:
int buff_3_closed = 0;
// Buffer 3, shared resource between plus sign thread and output thread:
char buffer_3[50000];
// Number of items in the buffer:
int count_3 = 0;
// Index where the plus sign thread will put the next item:
int produced_idx_3 = 0;
// Index where the plus sign thread will pick up the next item:
int consumed_idx_3 = 0;
// Initialize the mutex for buffer 2:
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2:
pthread_cond_t not_empty_3 = PTHREAD_COND_INITIALIZER;

/*
 Put an item in buffer_1
*/
void buff_1_PRODUCE(char item)
{
  // Lock the mutex before putting the item in the buffer:
  pthread_mutex_lock(&mutex_1);
  // Put the item in the buffer:
  buffer_1[produced_idx_1] = item;
  // Increment the index where the next item will be put.:
  produced_idx_1++;
  count_1++;
  // Signal to the consumer that the buffer is no longer empty:
  pthread_cond_signal(&not_empty_1);
  // Unlock the mutex:
  pthread_mutex_unlock(&mutex_1);
}

/*
 Function that reads user input and returns
 1(True) if equal to "STOP/n" returns 0(False) otherwise.
*/
int stop_processing(char letters[])
{
    if (letters[0] == 'S')
    {
        if (letters[1] == 'T')
        {
            if (letters[2] == 'O')
            {
                if (letters[3] == 'P')
                {
                    if (letters[4] == '\n')
                    {
                        return 1;
                    }
                }
            }
        }
    }       
    return 0;
}

/*
 Function that the input thread will run.
 Get input from the user.
 Put the item in the buffer shared with the line separator thread.
*/
void *get_input(void *args)
{   
    // While loop keeps the function running:
    while(1) 
    {
        // Get the user input:
        char character[1000];
        fgets(character, (MAX_CHAR * MAX_LINES), stdin);
        // Process each char of user input individually:
        for (int i = 0; i < strlen(character); i++)
        {            
            if (stop_processing(character))
            {
                keep_processing = 0;
                pthread_cond_signal(&not_empty_1);
                return NULL;
            }
            buff_1_PRODUCE(character[i]);
        }
    }
    return NULL;
}

/*
Get the next item from buffer_1
*/
char buff_1_CONSUME()
{
    // Lock the mutex before checking if the buffer has data:
    pthread_mutex_lock(&mutex_1);
    // Check for empty buffer:
    while (count_1 == 0)
    {
        if (!keep_processing)
        {
            return 0;
        }
        // Buffer is empty, wait for the producer to signal that the buffer has data:
        pthread_cond_wait(&not_empty_1, &mutex_1);
    }
    // Gets item:
    char item = buffer_1[consumed_idx_1];
    // Increment the index from which the item will be picked up:
    consumed_idx_1++;
    count_1--;
    // Unlock the mutex:
    pthread_mutex_unlock(&mutex_1);
    // Returns item:
    return item;
}

/*
 Put an item in buffer_2
*/
void buff_2_PRODUCE(char item)
{
    // Lock the mutex before putting the item in the buffer:
    pthread_mutex_lock(&mutex_2);
    // Put the item in the buffer:
    buffer_2[produced_idx_2] = item;
    // Increment the index where the next item will be put:
    produced_idx_2++;
    count_2++;
    // Signal to the consumer that the buffer is no longer empty:
    pthread_cond_signal(&not_empty_2);
    // Unlock the mutex:
    pthread_mutex_unlock(&mutex_2);
}

/*
 Function that the line separator thread will run. 
 Consume an item from buffer_1 shared with the input thread.
 Replace any newline character with a space.
 Produce an item in buffer_2 shared with the plus sign thread.
*/
void *separate_line(void *args)
{
    char item[1];
    char space = ' ';
    // Process each char individually:
    for (int i = 0; i < (MAX_CHAR*MAX_LINES); i++)
    {
        item[0] = buff_1_CONSUME();
        // Replaces newline character with a space:
        if (item[0] == '\n')
        {
            buff_2_PRODUCE(space);
        }
        // Adds new char to buffer_2:
        else 
        {
            buff_2_PRODUCE(item[0]);
        }
        // Check for empty buffer and "closed" flag:
        if ((!keep_processing) && (count_1 == 0))
        {
            // Update "closed" flag, signal, and end function:
            buff_1_closed = 1;
            pthread_cond_signal(&not_empty_2);
            return NULL;
        }
    }
    return NULL;
}

/*
Get the next item from buffer_2
*/
char buff_2_CONSUME()
{
    // Lock the mutex before checking if the buffer has data:
    pthread_mutex_lock(&mutex_2);
    // Check for empty buffer:
    while (count_2 == 0)
    {
        // Buffer is empty. Wait for the producer to signal that the buffer has data:
        pthread_cond_wait(&not_empty_2, &mutex_2);
    }
    // Gets item:
    char item = buffer_2[consumed_idx_2];
    // Increment the index from which the item will be picked up:
    consumed_idx_2++;
    count_2--;
    // Unlock the mutex:
    pthread_mutex_unlock(&mutex_2);
    // Return the item:
    return item;
}

/*
 Put an item in buffer_3
*/
void buff_3_PRODUCE(char item)
{
    // Lock the mutex before putting the item in the buffer:
    pthread_mutex_lock(&mutex_3);
    // Put the item in the buffer:
    buffer_3[produced_idx_3] = item;
    // Increment the index where the next item will be put:
    produced_idx_3++;
    count_3++;
    // Signal to the consumer that the buffer is no longer empty:
    pthread_cond_signal(&not_empty_3);
    // Unlock the mutex:
    pthread_mutex_unlock(&mutex_3);
}

/*
 Helper function for plusChange()
 that checks the next character present in buffer_2 
 and returns true if it is a "+", false if not.
*/
int check_next()
{
    
    // Lock the mutex before checking if the next is a plus.
    pthread_mutex_lock(&mutex_2);
    if (buffer_2[consumed_idx_2] == '+')
    {
        // Unlock the mutex
        pthread_mutex_unlock(&mutex_2);
        return 1;
    }
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_2);
    return 0;
}

/*
 Function that the plus sign thread will run. 
 Consume an item from buffer_2 (shared with the line separator thread)
 Replace any consecutive plusses (++) with a single "^".
 Produce an item in buffer_3 shared with the output thread.
*/
void *plusChange(void *args)
{
    char item[1];
    char carat = '^';
    // Process each char individually:
    for (int i = 0; i < (MAX_CHAR*MAX_LINES); i++)
    {
        item[0] = buff_2_CONSUME();
        // Replaces "++" with a carat:
        if (item[0] == '+')
        {
            // Checks if the next character in the buffer is a '+':
            if (check_next())
            {
                // Calls buff_2_CONSUME() to increment its index and decrement its count:
                buff_2_CONSUME();
                // Puts carat in buffer_3:
                buff_3_PRODUCE(carat);
            }
            // Adds new character to buffer_3:
            else
            {
                buff_3_PRODUCE(item[0]);
            }
        }
        // Adds new character to buffer_3:
        else
        {
            buff_3_PRODUCE(item[0]);
        }
        // Check for empty buffer and "closed" flag:
        if ((buff_1_closed) && (count_2 == 0))
        {
            // Update "closed" flag, signal, and end function:
            buff_2_closed = 1;
            pthread_cond_signal(&not_empty_3);
            return NULL;
        }
    }
    return NULL;
}

/*
Get the next item from buffer_3
*/
char buff_3_CONSUME()
{
    // Lock the mutex before checking if the buffer has data
    pthread_mutex_lock(&mutex_3);
    while (count_3 == 0)
    {
        // Buffer is empty. Wait for the producer to signal that the buffer has data
        pthread_cond_wait(&not_empty_3, &mutex_3);
    }
    // Gets item:
    char item = buffer_3[consumed_idx_3];
    // Increment the index from which the item will be picked up
    consumed_idx_3++;
    count_3--;
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_3);
    // Return the item
    return item;
}

/*
 Function that the output thread will run. 
 Consume an item from the buffer shared with the square root thread.
 Print the item.
*/
void *write_output(void *args)
{
    int lines = 0;
    char line_eighty[81];
    // Process each char individually:
    for (int i = 0; i < (MAX_CHAR*MAX_LINES); i++)
    {
        if (i < 80)
        {
            // Adds char to line candidate:
            line_eighty[i] = buff_3_CONSUME();
            // Prints line if it contains 80 characters:
            if ((i + 1) == 80){
                printf("%s\n", line_eighty);
                // Increments lines to track when 50 have been printed:
                lines++;
            }
            // Checks that buffer_3 is empty and will not be refilled:
            if ((buff_2_closed) && (count_3 == 0))
            {
                return NULL;
            }
        }
        if (i >= 80){
            // Adds char to line candidate:
            line_eighty[(i % 80)] = buff_3_CONSUME();
            // Prints line if it contains 80 new characters:
            if ((i + 1) % 80 == 0)
            {
                printf("%s\n", line_eighty);
                // Increments lines to track when 50 have been printed:
                lines++;
            }
            // Checks that buffer_3 is empty and will not be refilled:
            if ((buff_2_closed) && (count_3 == 0))
            {
                return NULL;
            }
        }
        // Checks if line maximum has been reached:
        if (lines >= MAX_LINES)
        {
            // Exits entire process since no more lines can be printed:
            exit(3);
            return NULL;
        }
    }
    return NULL;
}

int main()
{
    srand(time(0));
    pthread_t input_t, line_separate_t, plusses_t, output_t;
    // Create the threads
    pthread_create(&input_t, NULL, get_input, NULL);
    pthread_create(&line_separate_t, NULL, separate_line, NULL);
    pthread_create(&plusses_t, NULL, plusChange, NULL);
    pthread_create(&output_t, NULL, write_output, NULL);
    // Wait for the threads to terminate
    pthread_join(input_t, NULL);
    pthread_join(line_separate_t, NULL);
    pthread_join(plusses_t, NULL);
    pthread_join(output_t, NULL);
    return EXIT_SUCCESS;
}
