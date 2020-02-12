# mem_alloc
English:  bss-based memory-allocator for microcontroller, single-threaded

features: coalescence, private functions for freelist-control, block merge/-split

Comments in code are written in Dutch language

interface:

  mem_init():
	initialise memory pool
	
  mem_malloc(size):
	returns pointer to a memory block of size Bytes,
	or a NULL pointer on failure
	
  mem_calloc(nitems, itemsize):
	returns pointer to a zero-initialised memory block of nitems * itemsize Bytes,
	or a NULL pointer on failure
	
  mem_free(address):
	returns the memory block to the memory pool
	
  mem_realloc(address, newsize):
	with NULL-address, behaves like mem_malloc()
	with newsize 0, behaves like mem_free() and returns NULL
	otherwise the block will be shrinked or expanded if necessary, or moved or
	copied to a fresh acquired block, and the pointer to this block returned,
	or a NULL pointer if all the above failed
