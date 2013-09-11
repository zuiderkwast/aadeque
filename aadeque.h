/*
 * aadeque.h - Another array deque
 *
 * The author disclaims copyright to this source code.
 */
#include <stdlib.h>
#include <string.h>

/* allocation macros, tweakable */
#ifndef AADEQUE_ALLOC
	#define AADEQUE_ALLOC(size) malloc(size)
#endif
#ifndef AADEQUE_REALLOC
	#define AADEQUE_REALLOC(ptr, size, oldsize) realloc(ptr, size)
#endif
#ifndef AADEQUE_FREE
	#define AADEQUE_FREE(ptr, size) free(ptr)
#endif
#ifndef AADEQUE_OOM
	#define AADEQUE_OOM() exit(-1)
#endif

/* minimum capacity */
#ifndef AADEQUE_MIN_CAPACITY
	#define AADEQUE_MIN_CAPACITY 4
#endif

/* value type, tweakable */
#ifndef AADEQUE_VALUE_T
	#define AADEQUE_VALUE_T void*
#endif

/* the type of the lengths and indices */
#ifndef AADEQUE_SIZE_T
	#define AADEQUE_SIZE_T unsigned int
#endif

/*
 * Generics: prefix to use instead of 'oaht'. Defaults to oaht.
 */
#ifndef AADEQUE_PREFIX
	#define AADEQUE_PREFIX aadeque
#endif

/*
 * Macros to expand the prefix.
 */
#define AADEQUE_XXNAME(prefix, name) prefix ## name
#define AADEQUE_XNAME(prefix, name) AADEQUE_XXNAME(prefix, name)
#define AADEQUE_NAME(name) AADEQUE_XNAME(AADEQUE_PREFIX, name)

/* The deque type, optionally prefixed user-defined extra members */
struct AADEQUE_PREFIX {
	#ifdef AADEQUE_HEADER
	AADEQUE_HEADER
	#endif
	AADEQUE_SIZE_T cap;      /* capacity, actual length of the els array */
	AADEQUE_SIZE_T off;      /* offset to the first element in els */
	AADEQUE_SIZE_T len;      /* length */
	AADEQUE_VALUE_T els[1];  /* elements, allocated in-place */
};

/* aadeque_t is an alias for struct aadeque */
#undef AADEQUE_T
#define AADEQUE_T AADEQUE_NAME(_t)
typedef struct AADEQUE_PREFIX AADEQUE_T;

/* Size to allocate for a struct aadeque of capacity cap. Used internally. */
static inline size_t
AADEQUE_NAME(_sizeof)(AADEQUE_SIZE_T cap) {
	return sizeof(AADEQUE_T) + (cap - 1) * sizeof(AADEQUE_VALUE_T);
}

/*
 * Convert external index to internal one. Used internally.
 *
 * i must fulfil i >= 0 and i < length, otherwise the result is undefined.
 * i % cap == i & (cap - 1), since cap always is a power of 2.
 */
static inline AADEQUE_SIZE_T
AADEQUE_NAME(_idx)(AADEQUE_T *a, AADEQUE_SIZE_T i) {
	AADEQUE_SIZE_T idx = a->off + i;
	if (idx >= a->cap)
		idx -= a->cap;
	return idx;
}

/*
 * Creates an array of length len with undefined values.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_create)(AADEQUE_SIZE_T len) {
	AADEQUE_SIZE_T cap = len; //AADEQUE_MIN_CAPACITY;
	AADEQUE_T *a;
	//while (cap < len)
	//	cap = cap << 1;
	if (cap < AADEQUE_MIN_CAPACITY) cap = AADEQUE_MIN_CAPACITY;
	a = (AADEQUE_T *)AADEQUE_ALLOC(AADEQUE_NAME(_sizeof)(cap));
	if (!a) AADEQUE_OOM();
	a->len = len;
	a->off = 0;
	a->cap = cap;
	return a;
}

/*
 * Creates an empty array.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_create_empty)(void) {
	return AADEQUE_NAME(_create)(0);
}

/*
 * Frees the memory.
 */
static inline void
AADEQUE_NAME(_destroy)(AADEQUE_T *a) {
	AADEQUE_FREE(a, AADEQUE_NAME(_sizeof)(a->cap));
}

/*
 * Returns the number of elements in the array.
 */
static inline AADEQUE_SIZE_T
AADEQUE_NAME(_len)(AADEQUE_T *a) {
	return a->len;
}

/*
 * Fetch the element at the zero based index i. The index bounds are not
 * checked.
 */
static inline AADEQUE_VALUE_T
AADEQUE_NAME(_get)(AADEQUE_T *a, AADEQUE_SIZE_T i) {
	AADEQUE_SIZE_T pos = AADEQUE_NAME(_idx)(a, i);
	return a->els[pos];
}

/*
 * Set (replace) the element at the zero based index i. The index bounds are not
 * checked.
 */
static inline void
AADEQUE_NAME(_set)(AADEQUE_T *a, AADEQUE_SIZE_T i, AADEQUE_VALUE_T value) {
	AADEQUE_SIZE_T pos = AADEQUE_NAME(_idx)(a, i);
	a->els[pos] = value;
}

/*
 * Clear the memory (set to zery bytes) of n elements at indices between
 * i and i+n-1.
 *
 * Used internally if AADEQUE_CLEAR_UNUSED_MEM is defined.
 */
static inline void
AADEQUE_NAME(_clear)(AADEQUE_T *a, AADEQUE_SIZE_T i, AADEQUE_SIZE_T n) {
	if (AADEQUE_NAME(_idx)(a, i) > AADEQUE_NAME(_idx)(a, i + n - 1)) {
		/*
		 * It warps. There are two parts to clear.
		 *
		 *     0  i+n-1   i   cap
		 *    /   /      /   /
		 *   |--->      o---|
		 */
		memset(&a->els[0], 0,
		       sizeof(AADEQUE_VALUE_T) * AADEQUE_NAME(_idx)(a, n + i));
		memset(&a->els[AADEQUE_NAME(_idx)(a, i)], 0,
		       sizeof(AADEQUE_VALUE_T) * (a->cap - AADEQUE_NAME(_idx)(a, i)));
	}
	else {
		/*
		 * It's in consecutive memory.
		 *
		 *     0   i   i+n-1  cap
		 *    /   /     /    /
		 *   |   o----->    |
		 */
		memset(&a->els[AADEQUE_NAME(_idx)(a, i)], 0, sizeof(AADEQUE_VALUE_T) * n);
	}
}

/*
 * Clones an array deque, preserving the internal memory layout.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_clone)(AADEQUE_T *a) {
	size_t sz = AADEQUE_NAME(_sizeof)(a->cap);
	void *clone = AADEQUE_ALLOC(sz);
	if (!clone) AADEQUE_OOM();
	return (AADEQUE_T *)memcpy(clone, a, sz);
}

/*
 * Create an struct aadeque from the n values pointed to by array.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_from_array)(AADEQUE_VALUE_T *array, AADEQUE_SIZE_T n) {
	AADEQUE_T *a = AADEQUE_NAME(_create)(n);
	memcpy(a->els, array, sizeof(AADEQUE_VALUE_T) * n);
	return a;
}

/*
 * Compare the contents of a against a static C array of n elements. Returns 1
 * if the number of elements is equal to n and all elements are equal to their
 * counterparts in array, 0 otherwise.
 */
static inline int
AADEQUE_NAME(_eq_array)(AADEQUE_T *a, AADEQUE_VALUE_T *array,
                        AADEQUE_SIZE_T n) {
	AADEQUE_SIZE_T i;
	if (a->len != n)
		return 0;
	for (i = 0; i < n; i++)
		if (AADEQUE_NAME(_get)(a, i) != array[i])
			return 0;
	return 1;
}

/*----------------------------------------------------------------------------
 * Helpers for growing and compacting the underlying buffer. Like realloc,
 * these functions try to resize the underlying buffer and return a. It there
 * is not enough space, they copy the data to a new memory location, free the
 * old memory and return a pointer to the new the new memory allocation.
 *----------------------------------------------------------------------------*/

/*
 * Reserve space for at least n more elements
 */
static inline AADEQUE_T *
AADEQUE_NAME(_reserve)(AADEQUE_T *a, AADEQUE_SIZE_T n) {
	if (a->cap < a->len + n) {
		/* calulate and set new capacity */
		AADEQUE_SIZE_T oldcap = a->cap;
		do {
			a->cap = a->cap << 1;
		} while (a->cap < a->len + n);
		/* allocate more mem */
		a = (AADEQUE_T *)AADEQUE_REALLOC(a,
		                                 AADEQUE_NAME(_sizeof)(a->cap),
		                                 AADEQUE_NAME(_sizeof)(oldcap));
		if (!a) AADEQUE_OOM();
		/* adjust content to the increased capacity */
		if (a->off + a->len > oldcap) {
			/*
			 * It warped around before. Make it warp around the new boundary.
			 *
			 * Symbols: o-- = first part of content
			 *          --> = last part of content
			 *
			 *            0      oldcap     cap
			 *           /        /         /
			 * Before:  |-->  o--|
			 * After:   |-->     |      o--|
			 */
			memcpy(&(a->els[a->off + a->cap - oldcap]),
			       &(a->els[a->off]),
			       sizeof(AADEQUE_VALUE_T) * (a->cap - oldcap));
			#ifdef AADEQUE_CLEAR_UNUSED_MEM
			memset(&(a->els[a->off]), 0,
			       sizeof(AADEQUE_VALUE_T) * (a->cap - oldcap));
			#endif
			a->off += a->cap - oldcap;
		}
	}
	return a;
}

/*
 * Reduces the capacity to some number larger than or equal to mincap. Mincap
 * must be at least as large as the number of elements in the deque.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_compact_to)(AADEQUE_T *a, AADEQUE_SIZE_T mincap) {
	AADEQUE_SIZE_T doublemincap = 	mincap << 1;
	if (a->cap >= doublemincap && a->cap > AADEQUE_MIN_CAPACITY) {
		/*
		 * Halve the capacity as long as it is >= twice the minimum capacity.
		 */
		AADEQUE_SIZE_T oldcap = a->cap;
		/* Calulate and set new capacity */
		do {
			a->cap = a->cap >> 1;
		} while (a->cap >= doublemincap && a->cap > AADEQUE_MIN_CAPACITY);
		/* Adjust content to decreased capacity */
		if (a->off + a->len > oldcap) {
			/*
			 * It warpped around already. Move the first part to within the new
			 * boundaries.
			 *
			 * Symbols: o-- = first part of content
			 *          --> = last part of content
			 *
			 *            0       cap      oldcap
			 *           /        /         /
			 * Before:  |-->     |      o--|
			 * After:   |-->  o--|
			 */
			memcpy(&(a->els[a->off + a->cap - oldcap]),
			       &(a->els[a->off]),
			       sizeof(AADEQUE_VALUE_T) * (oldcap - a->off));
			a->off += a->cap - oldcap;
		}
		else if (a->off >= a->cap) {
			/*
			 * The whole content will be outside, but it's in one piece. Move
			 * it to offset 0.
			 *
			 *            0       cap      oldcap
			 *           /        /         /
			 * Before:  |        |  o----> |
			 * After:   |o---->  |
			 */
			memcpy(&(a->els[0]),
			       &(a->els[a->off]),
			       sizeof(AADEQUE_VALUE_T) * a->len);
			a->off = 0;
		}
		else if (a->off + a->len > a->cap) {
			/*
			 * It will overflow the new cap. Make it warp.
			 *
			 *            0       cap      oldcap
			 *           /        /         /
			 * Before:  |     o--|-->      |
			 * After:   |-->  o--|
			 */
			memcpy(&(a->els[0]),
			       &(a->els[a->cap]),
			       sizeof(AADEQUE_VALUE_T) * (a->off + a->len - a->cap));
		}
		else {
			/*
			 * The whole content is in the first half. Nothing to move.
			 *
			 *            0       cap      oldcap
			 *           /        /         /
			 * Before:  | o----> |         |
			 * After:   | o----> |
			 */
		}
		/* Free the unused, second half, between cap and oldcap. */
		a = (AADEQUE_T *)AADEQUE_REALLOC(a,
		                                 AADEQUE_NAME(_sizeof)(a->cap),
		                                 AADEQUE_NAME(_sizeof)(oldcap));
		if (!a) AADEQUE_OOM();
	}
	return a;
}

/*
 * Reduces the capacity to half if only 25% of the capacity or less is used.
 * Call this after removing elements. Automatically done by pop and shift.
 *
 * This strategy prevents the scenario that alternate insertions and deletions
 * trigger buffer resizing on every operation, thus keeping the insertions and
 * deletions at O(1) amortized.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_compact_some)(AADEQUE_T *a) {
	return AADEQUE_NAME(_compact_to)(a, a->len << 1);
}

/*----------------------------------------------------------------------------
 * Functions that extend the deque in either end. The added elements will not
 * be initialized, thus their indices will contain undefined values.
 *
 * These functions resize the underlying buffer when needed. Like realloc,
 * they try to resize the underlying buffer and return a. It there is not
 * enough space, they copy the data to a new memory location, free the old
 * memory and return a pointer to the new the new memory allocation.
 *----------------------------------------------------------------------------*/

/*
 * Inserts n undefined values after the last element.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_make_space_after)(AADEQUE_T *a, AADEQUE_SIZE_T n) {
	a = AADEQUE_NAME(_reserve)(a, n);
	a->len += n;
	return a;
}

/*
 * Inserts n undefined values before the first element.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_make_space_before)(AADEQUE_T *a, AADEQUE_SIZE_T n) {
	a = AADEQUE_NAME(_reserve)(a, n);
	a->off = AADEQUE_NAME(_idx)(a, a->cap - n);
	a->len += n;
	return a;
}

/*----------------------------------------------------------------------------
 * Functions for deleting multiple elements
 *----------------------------------------------------------------------------*/

/*
 * Deletes everything exept n elements starting at index i. Like slice, but
 * destructive, i.e. it modifies *a*. Returns a pointer to the new or modified
 * array deque.
 *
 * If length + offset is greater than the length of a, the behaviour is
 * undefined. No check is performed on *length* and *offset*.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_crop)(AADEQUE_T *a, AADEQUE_SIZE_T offset, AADEQUE_SIZE_T length) {
	#ifdef AADEQUE_CLEAR_UNUSED_MEM
	AADEQUE_NAME(_clear)(a, offset + length, a->len - length);
	#endif
	a->off = AADEQUE_NAME(_idx)(a, offset);
	a->len = length;
	return AADEQUE_NAME(_compact_some)(a);
}

/*
 * Deletes the last n elements.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_delete_last_n)(AADEQUE_T *a, AADEQUE_SIZE_T n) {
	return AADEQUE_NAME(_crop)(a, 0, a->len - n);
}

/*
 * Deletes the first n elements.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_delete_first_n)(AADEQUE_T *a, AADEQUE_SIZE_T n) {
	return AADEQUE_NAME(_crop)(a, n, a->len - n);
}

/*---------------------------------------------------------------------------
 * The pure deque operations: Inserting and deleting values in both ends.
 * Shift, unshift, push, pop.
 *---------------------------------------------------------------------------*/

/*
 * Insert an element at the beginning.
 * May change aptr if it needs to be reallocated.
 */
static inline void
AADEQUE_NAME(_unshift)(AADEQUE_T **aptr, AADEQUE_VALUE_T value) {
	*aptr = AADEQUE_NAME(_make_space_before)(*aptr, 1);
	(*aptr)->els[(*aptr)->off] = value;
}

/*
 * Remove an element at the beginning and return its value.
 * May change aptr if it needs to be reallocated.
*/
static inline AADEQUE_VALUE_T
AADEQUE_NAME(_shift)(AADEQUE_T **aptr) {
	AADEQUE_VALUE_T value = (*aptr)->els[(*aptr)->off];
	*aptr = AADEQUE_NAME(_crop)(*aptr, 1, (*aptr)->len - 1);
	return value;
}

/*
 * Insert an element at the end.
 * May change aptr if it needs to be reallocated.
 */
static inline void
AADEQUE_NAME(_push)(AADEQUE_T **aptr, AADEQUE_VALUE_T value) {
	*aptr = AADEQUE_NAME(_make_space_after)(*aptr, 1);
	(*aptr)->els[AADEQUE_NAME(_idx)(*aptr, (*aptr)->len - 1)] = value;
}

/*
 * Remove an element at the end and return its value.
 * May change aptr if it needs to be reallocated.
 */
static inline AADEQUE_VALUE_T
AADEQUE_NAME(_pop)(AADEQUE_T **aptr) {
	AADEQUE_VALUE_T value =
		(*aptr)->els[AADEQUE_NAME(_idx)(*aptr, (*aptr)->len - 1)];
	*aptr = AADEQUE_NAME(_crop)(*aptr, 0, (*aptr)->len - 1);
	return value;
}

/*---------------------------------------------------------------------------
 * Append or prepend all elements of one array deque to another
 *---------------------------------------------------------------------------*/

/*
 * Appends all elements of s2 to a1 and returns the (possibly reallocated) a1.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_append)(AADEQUE_T *a1, AADEQUE_T *a2) {
	AADEQUE_SIZE_T i = a1->len, j;
	a1 = AADEQUE_NAME(_make_space_after)(a1, a2->len);
	/* TODO: copy using memcpy instead of the loop */
	for (j = 0; j < a2->len; j++)
		AADEQUE_NAME(_set)(a1, i++, AADEQUE_NAME(_get)(a2, j));
	return a1;
}

/*
 * Prepends all elements of s2 to a1 and returns the (possibly reallocated) a1.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_prepend)(AADEQUE_T *a1, AADEQUE_T *a2) {
	AADEQUE_SIZE_T i = 0, j;
	a1 = AADEQUE_NAME(_make_space_before)(a1, a2->len);
	/* TODO: copy using memcpy instead of the loop */
	for (j = 0; j < a2->len; j++)
		AADEQUE_NAME(_set)(a1, i++, AADEQUE_NAME(_get)(a2, j));
	return a1;
}

/*---------------------------------------------------------------------------
 * Slice: copy a part of the contents to a new array deque.
 *---------------------------------------------------------------------------*/

/*
 * Creates a new array deque, by copying *length* elements starting at *offset*.
 *
 * If length + offset is greater than the length of a, the behaviour is
 * undefined. No check is performed on *length* and *offset*.
 *
 * See also *aadeque_crop()*.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_slice)(AADEQUE_T *a, AADEQUE_SIZE_T offset, AADEQUE_SIZE_T length) {
	AADEQUE_T *b = AADEQUE_NAME(_create)(length);
	/* TODO: use memcpy instead of the loop */
	AADEQUE_SIZE_T i;
	for (i = 0; i < length; i++) {
		AADEQUE_VALUE_T x = AADEQUE_NAME(_get)(a, i + offset);
		AADEQUE_NAME(_set)(b, i, x);
	}
	return b;
}

/*----------------------------------------------------------------------------
 * Various, perhaps less useful functions
 *----------------------------------------------------------------------------*/

/*
 * Reduces the capacity, freeing as much unused memory as possible. This is
 * only useful after deleting elements, if you're not going to add elements the
 * deque again (which may trigger expensive resizing again).
 *
 * Returns the unmodified pointer if no memory could be free'd. Otherwise, frees
 * the old one and returns a pointer to the new, resized allocation.
 */
static inline AADEQUE_T *
AADEQUE_NAME(_compact)(AADEQUE_T *a) {
	return AADEQUE_NAME(_compact_to)(a, a->len);
}

/*
 * Joins the parts together in memory, possibly in the wrong order. This is
 * useful if you want to sort or shuffle the underlaying array using functions
 * for raw arrays (such as qsort).
 */
static inline void
AADEQUE_NAME(_make_contiguous_unordered)(AADEQUE_T *a) {
	if (a->off + a->len > a->cap) {
		/*
		 * It warps around. Just move the parts together in the wrong order.
		 *
		 *                  0  end  start cap
		 *                 /  /    /     /
		 * Before:        |-->    o-----|
		 * Intermediate:  |-->o-----    |
		 * After:         |o------->    |
		 */
		memmove(&(a->els[a->off + a->len - a->cap]),
		        &(a->els[a->off]),
		        sizeof(AADEQUE_VALUE_T) * (a->cap - a->off));
		a->off = 0;
	}
}
