/*
 * Test with:
 *   echo 123456 | valgrind --leak-check=yes ./a.out
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dmalloc.h>

int *fun() {
  int        var;
  int        *stck = &var;
  int        *heap = malloc(2 * sizeof(int));
  static int *bss;
                                  //                                                          val mudfl duma dmalloc
  printf("%d\n", var);            // var hasn't been initialized                               O   N     N     N
  printf("%d\n", heap[0]);        // heap hasn't been initialized                              O   O     N     N
  heap[2] = 42;                   // wrote past the end of the block                           O   O     O     O
  printf("%d\n", heap[-1]);       // read from before start of the block                       O   O     O     N
  printf("%d\n", stck[-15]);      // reading from a bad stack location, outside current frame  N   O     N     N
  printf("%d\n", stck[-3]);       // reading from a bad stack location, inside currant frame   N   O     N     N
  read(0, heap, 10);              // buffer passed to syscall is too small                     O   N     N     N
  read(0, stck, 10);              // buffer passed to syscall is too small                     N   N     N     N
  free(heap);
  printf("%d\n", *heap);          // heap was already freed                                    O   O     O     N
  heap = malloc(sizeof(int));     // memory leaks (definitely lost)                            O   O    N      O
  stck = heap;                    // .. even if another variable point on it                   O   O    N      O
  bss = malloc(sizeof(int));      // memory leaks (still reachable)                            O   /    N      O
  bss[0] = 1;
  var = 1;
  return NULL;                    // return reference to local object (detected during compil) /   /    N      N
}

int main(int argc, char **argv) {
  int *var;
  var = fun();
  printf("%d\n", *var);
  return 0;
}

