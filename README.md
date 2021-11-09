# valdival_program4

Multi-threaded Producer Consumer Pipeline

A program that creates 4 threads to process input from standard input as follows

Thread 1, called the Input Thread, reads in lines of characters from the standard input.
Thread 2, called the Line Separator Thread, replaces every line separator in the input with a space.
Thread 3, called the Plus Sign thread, replaces every pair of plus signs, i.e., "++", with a "^".
Thread 4, called the Output Thread, write this processed data to standard output as lines of exactly 80 characters.

Furthermore, in your program these 4 threads must communicate with each other using the Producer-Consumer approach. 
