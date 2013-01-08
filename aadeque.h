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

/* minimum capacity, tweakable, must be a power of 2 */
#ifndef AADEQUE_MIN_CAPACITY
	#define AADEQUE_MIN_CAPACITY 4
#endif

/* value type, tweakable */
#ifndef AADEQUE_VALUE_TYPE
	#define AADEQUE_VALUE_TYPE void*
#endif

/* The deque type, optionally prefixed user-defined extra members */
typedef struct {
	#ifdef AADEQUE_HEADER
	AADEQUE_HEADER
	#endif
	unsigned int cap;           /* capacity, actual length of the els array */
	unsigned int off;           /* offset to the first element in els */
	unsigned int len;           /* length */
	AADEQUE_VALUE_TYPE els[1];  /* elements, allocated in-place */
} aadeque_t;

/* Size to allocate for an aadeque_t of capacity cap. Used internally. */
static inline size_t aadeque_sizeof(unsigned int cap) {
	return sizeof(aadeque_t) + (cap - 1) * sizeof(AADEQUE_VALUE_TYPE);
}

/*
 * Convert external index to internal one. Used internally.
 *
 * i % cap == i & (cap - 1), since cap always is a power of 2.
 */
static inline unsigned int aadeque_idx(aadeque_t *a, unsigned int i) {
	return (a->off + i) & (a->cap - 1);
}

/*
 * Creates an array of length len with undefined values.
 */
static inline aadeque_t *aadeque_create(unsigned int len) {
	unsigned int cap = AADEQUE_MIN_CAPACITY;
	while (cap < len)
		cap = cap << 1;
	aadeque_t *arr = (aadeque_t *)AADEQUE_ALLOC(aadeque_sizeof(cap));
	arr->len = len;
	arr->off = 0;
	arr->cap = cap;
	return arr;
}

/*
 * Creates an empty array.
 */
static inline aadeque_t *aadeque_create_empty(void) {
	return aadeque_create(0);
}

/*
 * Frees the memory.
 */
static inline void aadeque_destroy(aadeque_t *a) {
	AADEQUE_FREE(a, aadeque_sizeof(a->cap));
}

/*
 * Returns the number of elements in the array.
 */
static inline unsigned int aadeque_len(aadeque_t *a) {
	return a->len;
}

/*
 * Fetch the element at the zero based index i. The index bounds are not
 * checked.
 */
static inline AADEQUE_VALUE_TYPE aadeque_get(aadeque_t *a, unsigned int i) {
	unsigned int pos = aadeque_idx(a, i);
	return a->els[pos];
}

/*
 * Set (replace) the element at the zero based index i. The index bounds are not
 * checked.
 */
static inline void aadeque_set(aadeque_t *a, unsigned int i,
                               AADEQUE_VALUE_TYPE value) {
	unsigned int pos = aadeque_idx(a, i);
	a->els[pos] = value;
}

/*
 * Clear the memory (set to zery bytes) of n elements at indices between
 * i and i+n-1.
 *
 * Used internally if AADEQUE_CLEAR_UNUSED_MEM is defined.
 */
static inline void aadeque_clear(aadeque_t *a, unsigned int i, unsigned int n) {
	if (aadeque_idx(a, i) > aadeque_idx(a, i + n - 1)) {
		/*
		 * It warps. There are two parts to clear.
		 *
		 *     0  i+n-1   i   cap
		 *    /   /      /   /
		 *   |--->      o---|
		 */
		memset(&a->els[0], 0,
		       sizeof(AADEQUE_VALUE_TYPE) * aadeque_idx(a, n + i));
		memset(&a->els[aadeque_idx(a, i)], 0,
		       sizeof(AADEQUE_VALUE_TYPE) * (a->cap - aadeque_idx(a, i)));
	}
	else {
		/*
		 * It's in consecutive memory.
		 *
		 *     0   i   i+n-1  cap
		 *    /   /     /    /
		 *   |   o----->    |
		 */
		memset(&a->els[aadeque_idx(a, i)], 0, sizeof(AADEQUE_VALUE_TYPE) * n);
	}
}

/*----------------------------------------------------------------------------
 * Helpers for growing and compacting the underlying buffer
 *----------------------------------------------------------------------------*/

/*
 * Reserve space for at least n more elements
 */
static inline void aadeque_reserve(aadeque_t **aptr, unsigned int n) {
	aadeque_t *a = *aptr;
	if (a->cap < a->len + n) {
		/* calulate and set new capacity */
		unsigned int oldcap = a->cap;
		do {
			a->cap = a->cap << 1;
		} while (a->cap < a->len + n);
		/* allocate more mem */
		a = (aadeque_t *)AADEQUE_REALLOC(a,
		                                 aadeque_sizeof(a->cap),
		                                 aadeque_sizeof(oldcap));
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
			       sizeof(AADEQUE_VALUE_TYPE) * (a->cap - oldcap));
			#ifdef AADEQUE_CLEAR_UNUSED_MEM
			memset(&(a->els[a->off]), 0,
			       sizeof(AADEQUE_VALUE_TYPE) * (a->cap - oldcap));
			#endif
			a->off += a->cap - oldcap;
		}
	}
	*aptr = a;
}

/*
 * Reduces the capacity to a number larger than or equal to mincap. Mincap must
 * be at least as large as the number of elements in the deque.
 */
static inline void aadeque_compact_to(aadeque_t **aptr, unsigned int mincap) {
	aadeque_t *a = *aptr;
	unsigned int doublemincap = 	mincap << 1;
	if (a->cap >= doublemincap && a->cap > AADEQUE_MIN_CAPACITY) {
		/*
		 * Halve the capacity as long as it is >= twice the minimum capacity.
		 */
		unsigned int oldcap = a->cap;
		/* Calulate and set new capacity */
		do {
			a->cap = a->cap >> 1;
		} while (a->cap >= doublemincap && a->cap > AADEQUE_MIN_CAPACITY);
		/* Adjust content to decreased capacity */
		if (a->off + a->len >= oldcap) {
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
			       sizeof(AADEQUE_VALUE_TYPE) * (oldcap - a->off));
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
			       sizeof(AADEQUE_VALUE_TYPE) * a->len);
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
			       sizeof(AADEQUE_VALUE_TYPE) * (a->off + a->len - a->cap));
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
		a = (aadeque_t *)AADEQUE_REALLOC(a,
		                                 aadeque_sizeof(a->cap),
		                                 aadeque_sizeof(oldcap));
	}
	*aptr = a;
}

/*
 * Reduces the capacity to half if only 25% of the capacity or less is used.
 * Call this after removing elements. Automatically done by pop and shift.
 *
 * This strategy prevents the scenario that alternate insertions and deletions
 * trigger buffer resizing on every operation, thus keeping the insertions and
 * deletions at O(1) amortized.
 */
static inline void aadeque_compact_some(aadeque_t **aptr) {
	aadeque_compact_to(aptr, (*aptr)->len << 1);
}

/*
 * Reduces the capacity by halving it as many times as possible, freeing unused
 * memory.
 */
static inline void aadeque_compact(aadeque_t **aptr) {
	aadeque_compact_to(aptr, (*aptr)->len);
}

/*
 * Joins the parts together in memory, possibly in the wrong order. This is
 * useful if you want to sort or shuffle the underlaying array using functions
 * for raw arrays.
 */
static inline void aadeque_make_contiguous_unordered(aadeque_t *a) {
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
		        sizeof(AADEQUE_VALUE_TYPE) * (a->cap - a->off));
		a->off = 0;
	}
}

/*---------------------------------------------------------------------------
 * Resize the deque by deleting existing or inserting undefined values in
 * each end. Resizes the underlying buffer when needed.
 *---------------------------------------------------------------------------*/

/*
 * Deletes the last n elements.
 */
static inline void aadeque_delete_last_n(aadeque_t **aptr, unsigned int n) {
	#ifdef AADEQUE_CLEAR_UNUSED_MEM
	aadeque_clear(*aptr, (*aptr)->len - n, n);
	#endif
	(*aptr)->len -= n;
	aadeque_compact_some(aptr);
}

/*
 * Deletes the first n elements.
 */
static inline void aadeque_delete_first_n(aadeque_t **aptr, unsigned int n) {
	#ifdef AADEQUE_CLEAR_UNUSED_MEM
	aadeque_clear(*aptr, 0, n);
	#endif
	(*aptr)->off = aadeque_idx(*aptr, n);
	(*aptr)->len -= n;
	aadeque_compact_some(aptr);
}

/*
 * Inserts n undefined values after the last element.
 */
static inline void aadeque_make_space_after(aadeque_t **aptr, unsigned int n) {
	aadeque_reserve(aptr, n);
	(*aptr)->len += n;
}

/*
 * Inserts n undefined values before the first element.
 */
static inline void aadeque_make_space_before(aadeque_t **aptr, unsigned int n) {
	aadeque_reserve(aptr, n);
	(*aptr)->off = aadeque_idx(*aptr, (*aptr)->cap - 1);
	(*aptr)->len += n;
}

/*---------------------------------------------------------------------------
 * The pure deque operations: Inserting and deleting values in both ends.
 * Shift, unshift, push, pop.
 *---------------------------------------------------------------------------*/

/*
 * Insert an element at the beginning.
 * May change array ptr if it needs to be reallocated.
 */
static inline void aadeque_unshift(aadeque_t **aptr, AADEQUE_VALUE_TYPE value) {
	aadeque_make_space_before(aptr, 1);
	(*aptr)->els[(*aptr)->off] = value;
}

/*
 * Remove an element at the beginning and return its value.
 * May change array ptr if it needs to be reallocated.
*/
static inline AADEQUE_VALUE_TYPE aadeque_shift(aadeque_t **aptr) {
	AADEQUE_VALUE_TYPE value = (*aptr)->els[(*aptr)->off];
	aadeque_delete_first_n(aptr, 1);
	return value;
}

/*
 * Insert an element at the end.
 * May change array ptr if it needs to be reallocated.
 */
static inline void aadeque_push(aadeque_t **aptr, AADEQUE_VALUE_TYPE value) {
	aadeque_make_space_after(aptr, 1);
	(*aptr)->els[aadeque_idx(*aptr, (*aptr)->len - 1)] = value;
}

/*
 * Remove an element at the end and return its value.
 * May change array ptr if it needs to be reallocated.
 */
static inline AADEQUE_VALUE_TYPE aadeque_pop(aadeque_t **aptr) {
	AADEQUE_VALUE_TYPE value =
		(*aptr)->els[aadeque_idx(*aptr, (*aptr)->len - 1)];
	aadeque_delete_last_n(aptr, 1);
	return value;
}
