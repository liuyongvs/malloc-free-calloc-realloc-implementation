# malloc-free-calloc-realloc-implementation
Implement malloc/free/calloc/realloc according to the tutorial.

In fact, it is a memory pool.I strongly suggest everyone read it.
You will understand memory deeply.
Believe me!

And i test 32/64 bits computer and find that they both allocate the 33 page(33 * 4KB = 132KB) memory size poll.
Then the malloc will get the size by the memory pool.
It will decrease the overhead of create and revocation, which will interact with the OS.
