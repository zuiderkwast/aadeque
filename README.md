aadeque: Another array deque
============================

An array deque is a dynamic array with fast insertion and deletion in both ends.

It can be used much like a dynamic array and as a FIFO queue. It is implemented
as a circular buffer that grows and shrinks automatically. This is developed as
part of the runtime for a language I'm working on.

The implementation consists of a single `.h` file of standard C code. It
compiles cleanly with `-Wall -pedantic`. All functions are small and declared
`static inline`. This design has the benefit that it's easy to use (just
one file to include) and that any unused functions will not take up space in the
executable. This design also allows tweaking, by defining some macros. See
*Tweaking macros* below.

The use of modulo for index lookups is avoided by using only powers of 2 as
the possible capacities. Only one contigous area of memory is used for one array
deque (for offset, length, capacity and the contents itself).

Usage
-----

``` C
#include "aadeque.h"
```

The type of an array deque is `aadeque_t`. The values are of type
`AADEQUE_VALUE_TYPE` which is `void *` by default, but can be of defined to any
type you want. See *Tweaking macros* below.

Create, destroy, get, set, length
---------------------------------

Functions for creating and freeing, checking the length, accessing elements by
index and replacing elements by index. These are all constant time operations.

``` C
static inline aadeque_t *
aadeque_create_empty(void);

static inline void
aadeque_destroy(aadeque_t *a);

static inline unsigned int
aadeque_len(aadeque_t *a);

static inline AADEQUE_VALUE_TYPE
aadeque_get(aadeque_t *a, unsigned int i);

static inline void
aadeque_set(aadeque_t *a, unsigned int i, AADEQUE_VALUE_TYPE value);
```

Push, pop, shift, unshift
-------------------------

The basic operations of an array deque, i.e. double ended queue, for inserting
and deleting elements in both ends. These are amortized O(1).

``` C
static inline void
aadeque_unshift(aadeque_t **aptr, AADEQUE_VALUE_TYPE value);

static inline AADEQUE_VALUE_TYPE
aadeque_shift(aadeque_t **aptr);

static inline void
aadeque_push(aadeque_t **aptr, AADEQUE_VALUE_TYPE value);

static inline AADEQUE_VALUE_TYPE
aadeque_pop(aadeque_t **aptr);
```

These take a pointer to a pointer to the array deque, because they may need to
reallocate it and thus change the location of the array deque in memory.

Appending and prepending
------------------------

``` C
static inline aadeque_t *
aadeque_append(aadeque_t *a1, aadeque_t *a2);

static inline aadeque_t *
aadque_prepend(aadeque_t *a1, aadeque_t *a2);
```

Appends or prepends all elements of *a2* to *a1*.

They try to resize *a1* so that all elements fit in its memory and return the
same pointer. If there is not enough space, they move the data of *a1* to
another memory allocation, free the old pointer and return a pointer to the new
allocation.

Deleting multiple
-----------------

``` C
static inline aadeque_t *
aadeque_delete_last_n(aadeque_t *a, unsigned int n);

static inline aadeque_t *
aadeque_delete_first_n(aadeque_t *a, unsigned int n);
```

These return *a* or a pointer to another memory location if the allocation has
been changed to reduce its size.

Resizing by inserting undefined values
--------------------------------------

The `aadeque_make_space_` functions grow the array deque by "inserting"
undefined values in the beginning and the end respectively.

``` C
static inline aadeque_t *
aadeque_make_space_after(aadeque_t *a, unsigned int n);

static inline aadeque_t *
aadeque_make_space_before(aadeque_t *a, unsigned int n);
```

It's also possible to create a non-empty array deque of undefined values, with
the desired length *len*:

``` C
static inline aadeque_t *
aadeque_create(unsigned int len);
```

For more functions, see the source code. It is well commented.

Tweaking macros
---------------

It's possible to tweak some of the behaviour by defining some macros prior to
including `aadeque.h`. This is entirely optional.

`AADEQUE_VALUE_TYPE`: Define this to the type you want to store in your array
deque. Defaults to `void *`.

`AADEQUE_HEADER`: Define this if you want to include your own fields in the
`aadeque_t`, which is a struct. (A tag? A reference-counter?)

To use a custom allocator you may define `AADEQUE_ALLOC(size)`,
`AADEQUE_REALLOC(ptr, size, oldsize)` and `AADEQUE_FREE(ptr, size)` to your
custom allocation functions. By default `malloc(size)`, `realloc(ptr, size)`
and `free(ptr)` are used.

`AADEQUE_OOM()` is called when a memory allocation fails, which normally means
that we're out of memory. Define this macro if you want to handle this. The
default is `exit(-1)`.

Defining `AADEQUE_CLEAR_UNUSED_MEM` causes unused allocated memory to be
overwritten with nul bytes. This might be useful if you're using a conservative
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
	/* Store some extra stuff in the header */
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
