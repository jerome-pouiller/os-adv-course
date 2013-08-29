/*
 * Test with:
 *   echo "core.%e.%p" | sudo tee /proc/sys/kernel/core_pattern
 *   ulimit -c unlimited
 * dmalloc:
 *   Paquet ubuntu bugué: https://bugs.launchpad.net/ubuntu/+source/dmalloc/+bug/971174
 *   wget http://dmalloc.com/releases/dmalloc-5.5.2.tgz
 *   tar xvzf dmalloc-5.5.2.tgz
 *   Suppression des surcharges des fonctions dans malloc.c
 *   ./configure && make && sudo make install
 *   gcc -g debug_mem.c -ldmalloc -DDMALLOC -o debug_dmalloc
 *   dmalloc -DV
 *   dmalloc -tV
 *   dmalloc high -m error-abort -l dmalloc.out
 *   echo 123456 | DMALLOC_OPTIONS=debug=0xcb4ed2b,log=dmalloc.out ./debug_dmalloc
 *   less dmalloc.out
 * DUMA:
 *   gcc -g debug_mem.c -o debug_duma
 *   echo 1234 | LD_PRELOAD=/usr/lib/libduma.so.0.0.0 DUMA_FILL=1 DUMA_PROTECT_FREE=1 ./debug_duma
 *   gdb -core core.debug_duma.xxxxx debug_duma
 * Valgrind:
 *   gcc -g debug_mem.c -o debug_valgrind   
 *   echo 123456 | valgrind --leak-check=yes ./debug_valgrind
 * Mudflap
 *   sudo apt-get install libmudflap0-4.6-dev
 *   gcc -fmudflap -lmudflap -g debug_mem.c -o debug_mudflap
 *   export MUDFLAP_OPTIONS='-help';  echo 12345 | ./debug_mudflap
 *   export MUDFLAP_OPTIONS='-print-leaks -check-initialization'; echo 12345 | ./debug_mudflap
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef DMALLOC
#include <dmalloc.h>
#endif

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

