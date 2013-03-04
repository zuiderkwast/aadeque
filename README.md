aadeque: Another array deque
============================

An array deque is a dynamic array with fast insertion and deletion in both ends.

It can be used much like a dynamic array and as a FIFO queue. It is implemented
as a circular buffer that grows and shrinks automatically. A contiguous area of
memory is used for one array deque (for offset, length, capacity and the
contents itself). It is developed for use in the runtime for a language I'm
working on.

The implementation consists of a single `.h` file of standard C code. It
compiles cleanly with `-Wall -pedantic -std=c99`. All functions are small and
declared `static inline`. This design has the benefit that it's easy to use
(just one file to include) and that any unused functions will not take up space
in the executable. This design also allows tweaking, by defining some macros.
See *Tweaking macros* below.

Usage
-----

``` C
#include "aadeque.h"
```

The type of an array deque is `struct aadeque`. The values are of type
`AADEQUE_VALUE_T` which is a macro defined to `void *` by default, but can be
defined to any type you want. The length and indices of the array deque has
type `AADEQUE_SIZE_T`, which is a macro that is `unsigned int` by default. See
*Tweaking macros* below.

Create, destroy, get, set, length
---------------------------------

Functions for creating and freeing:

``` C
static inline struct aadeque *
aadeque_create_empty(void);

static inline struct aadeque *
aadeque_create(AADEQUE_SIZE_T len);

static inline void
aadeque_destroy(struct aadeque *a);
```

`aadeque_create` creates a array deque with `len` undefined values.

Below are functions for checking the length, accessing elements by index and
replacing elements by index. These are all constant time operations.

``` C
static inline AADEQUE_SIZE_T
aadeque_len(struct aadeque *a);

static inline AADEQUE_VALUE_T
aadeque_get(struct aadeque *a, AADEQUE_SIZE_T i);

static inline void
aadeque_set(struct aadeque *a, AADEQUE_SIZE_T i, AADEQUE_VALUE_T value);
```

Push, pop, shift, unshift
-------------------------

The basic operations of an array deque, i.e. double ended queue, for inserting
and deleting elements in both ends. These are amortized O(1).

``` C
static inline void
aadeque_unshift(struct aadeque **aptr, AADEQUE_VALUE_T value);

static inline AADEQUE_VALUE_T
aadeque_shift(struct aadeque **aptr);

static inline void
aadeque_push(struct aadeque **aptr, AADEQUE_VALUE_T value);

static inline AADEQUE_VALUE_T
aadeque_pop(struct aadeque **aptr);
```

These take a pointer to a pointer to the array deque, because they may need to
reallocate it and thus change the location of the array deque in memory.

Append and prepend
------------------

``` C
static inline struct aadeque *
aadeque_append(struct aadeque *a1, struct aadeque *a2);

static inline struct aadeque *
aadque_prepend(struct aadeque *a1, struct aadeque *a2);
```

Appends or prepends all elements of *a2* to *a1*.

They try to resize *a1* so that all elements fit in its memory and return the
same pointer. If there is not enough space, they move the data of *a1* to
another memory allocation, free the old pointer and return a pointer to the new
allocation.

Delete multiple
---------------

``` C
static inline struct aadeque *
aadeque_crop(struct aadeque *a, AADEQUE_SIZE_T offset, AADEQUE_SIZE_T length);

static inline struct aadeque *
aadeque_delete_last_n(struct aadeque *a, AADEQUE_SIZE_T n);

static inline struct aadeque *
aadeque_delete_first_n(struct aadeque *a, AADEQUE_SIZE_T n);
```

These return *a* or a new pointer if the resulting array deque has been moved
to a new memory location.

*aadeque_crop()* shrinks the array deque by deleting all elements except the
*length* elements in the interval from *offset* to *offset* + *length* - 1. See
also *aadeque_slice()*.

Slice
-----

Copy a part (a slice) of the contents to a new array deque.

``` C
static inline struct aadeque *
aadeque_slice(struct aadeque *a, AADEQUE_SIZE_T offset, AADEQUE_SIZE_T length);
```

Creates a new array deque, by copying *length* elements starting at *offset*.

If length + offset is greater than the length of *a*, the behaviour is
undefined. No check is performed on *length* and *offset*.

Resizing by inserting undefined values
--------------------------------------

The `aadeque_make_space_` functions grow the array deque by "inserting"
undefined values in the beginning and the end respectively.

``` C
static inline struct aadeque *
aadeque_make_space_after(struct aadeque *a, AADEQUE_SIZE_T n);

static inline struct aadeque *
aadeque_make_space_before(struct aadeque *a, AADEQUE_SIZE_T n);
```

For more functions, see the source code. It is well commented.

Tweaking macros
---------------

It's possible to tweak some of the behaviour by defining some macros prior to
including `aadeque.h`. This is entirely optional.

`AADEQUE_VALUE_T`: Define this to the type you want to store in your array
deque. Defaults to `void *`.

`AADEQUE_SIZE_T`: The integer type used for lengths and indices. Defaults to
`unsigned int`.

`AADEQUE_HEADER`: Define this if you want to include your own fields in the
`struct aadeque`. (A tag? A reference-counter?)

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
#define AADEQUE_VALUE_T int
#define AADEQUE_HEADER int extra1; int extra2;
#define AADEQUE_MIN_CAPACITY 8
#include "aadeque.h"

#include <stdio.h>

int main() {
	int i;
	struct aadeque *a = aadeque_create_empty();
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

Public domain
-------------

The author disclaims copyright to this source code.
