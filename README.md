aadeque
=======

Another array deque.

Overview
--------

An array deque is a dymaic array with fast insertion and deletion in both ends.

It can be used much like a dynamic array, likewise as a FIFO queue. It is implemented as a circular buffer that grows and shrinks automatically. This is
developed as the built-in array type for a language I'm working on.

The implementation consists of a single `.h` file of standard C code. It compiles cleanly with `-Wall -pedantic`. All functions are small and declared
`static inline`. This design has the benefit that it's easy to use (just
include one file) and that any unused functions will not take up space in the
executable. This design also allows tweaking, by defining some macros. See
*Tweaking macros* below.

The use of modulo for index lookups is avoided by using only powers of 2 as
the possible capacities. Only one contigous area of memory is used for one array
deque (for offset, length, capacity and the contents itself).

Usage
-----

`aadeque_t`: The type of an array deque. It's a struct. The values are of type
`AADEQUE_VALUE_TYPE` which is `void *` by default, but can be of defined to any
type you want. See *Tweaking macros* below.

Functions for creating and freeing, check the length, accessing elements by
index and replacing elements by index:

``` C
static inline aadeque_t *aadeque_create_empty(void);
static inline void aadeque_destroy(aadeque_t *a);
static inline unsigned int aadeque_len(aadeque_t *a);
static inline AADEQUE_VALUE_TYPE aadeque_get(aadeque_t *a, unsigned int i);
static inline void aadeque_set(aadeque_t *a, unsigned int i,
                               AADEQUE_VALUE_TYPE value);
```

The basic functions for adding and deleting elements in the beginning and the
end of the array deque are `unshift`, `shift`, `push` and `pop`.

``` C
static inline void aadeque_unshift(aadeque_t **aptr, AADEQUE_VALUE_TYPE value);
static inline AADEQUE_VALUE_TYPE aadeque_shift(aadeque_t **aptr);
static inline void aadeque_push(aadeque_t **aptr, AADEQUE_VALUE_TYPE value);
static inline AADEQUE_VALUE_TYPE aadeque_pop(aadeque_t **aptr);
```

These take a pointer to a pointer to the array deque, because they may need to
reallocate it and thus change the location of the array deque in memory.

To avoid repeded use of push, pop, shift and unshift, there are functions that
add and delete multiple elements at once. The `aadeque_make_space_` functions
grow the array deque by "inserting" undefined values in the beginning and the
end respectively.

``` C
static inline void aadeque_delete_last_n(aadeque_t **aptr, unsigned int n);
static inline void aadeque_delete_first_n(aadeque_t **aptr, unsigned int n);
static inline void aadeque_make_space_after(aadeque_t **aptr, unsigned int n);
static inline void aadeque_make_space_before(aadeque_t **aptr, unsigned int n);
```

It's also possible to create a non-empty array deque, with the desired length `len`, initially containing undefined values:

``` C
static inline aadeque_t *aadeque_create(unsigned int len);
```

For more functions, see the source code. It is well commented.

Tweaking macros
---------------

It's possible to tweak some of the behaviour by defining some macros prior to
including `aadeque.h`. This is entirely optional.

`AADEQUE_VALUE_TYPE`: Define this to the type you want to store in your array
deque. Defaults to `void *`.

`AADEQUE_HEADER`: Define this if you want to include your own fields in the
`aadeque_t` struct. (A tag? A reference-counter?)

Allocation. You may define `AADEQUE_ALLOC(size)`,
`AADEQUE_REALLOC(ptr, size, oldsize)` and `AADEQUE_FREE(ptr, size)` to your
custom allocation functions. By default `malloc(size)`, `realloc(ptr, size)`
and `free(ptr)` are used.

Defining `AADEQUE_CLEAR_UNUSED_MEM` causes unused allocated memory to be overwritten with nul bytes. This might be useful if you're using a conservative
garbage collector together with these array deques.

The minimum capacity `AADEQUE_MIN_CAPACITY` can be defined to any power of 2.
The default is 4.

Examples
--------

``` C
#define AADEQUE_VALUE_TYPE int
#define AADEQUE_HEADER int extra1; int extra2;
#define AADEQUE_MIN_CAPACITY 8
#include "aadeque.h"

#include <stdio.h>

int main() {
	int i;
	aadeque_t *a = aadeque_create_empty();
	a->extra1 = 42;
	a->extra2 = 999;
	/* Add the numbers 0 to 9 */
	for (i = 0; i < 10; i++) {
		aadeque_push(&a, i);
	}
	/* Remove the elements in reverse order and print them */
	while (aadeque_len(a) > 0) {
		int x = aadeque_shift(&a);
		printf("Got %d\n", x);
	}
	printf("The extra data in the header is %d and %d.\n",
	       a->extra1, a->extra2);
	aadeque_destroy(a);
	return 0;
}
```

Todo
----

More complete tests.

Public domain
-------------

The author disclaims copyright to this source code.
