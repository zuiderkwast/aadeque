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

/* minimum capacity, tweakable, must be a power of 2 */
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

/* The deque type, optionally prefixed user-defined extra members */
struct aadeque {
	#ifdef AADEQUE_HEADER
	AADEQUE_HEADER
	#endif
	AADEQUE_SIZE_T cap;      /* capacity, actual length of the els array */
	AADEQUE_SIZE_T off;      /* offset to the first element in els */
	AADEQUE_SIZE_T len;      /* length */
	AADEQUE_VALUE_T els[1];  /* elements, allocated in-place */
};

/* An alias for struct aadeque (deprecated) */
typedef struct aadeque aadeque_t;

/* Size to allocate for a struct aadeque of capacity cap. Used internally. */
static inline size_t
aadeque_sizeof(AADEQUE_SIZE_T cap) {
	return sizeof(struct aadeque) + (cap - 1) * sizeof(AADEQUE_VALUE_T);
}

/*
 * Convert external index to internal one. Used internally.
 *
 * i % cap == i & (cap - 1), since cap always is a power of 2.
 */
static inline AADEQUE_SIZE_T
aadeque_idx(struct aadeque *a, AADEQUE_SIZE_T i) {
	return (a->off + i) & (a->cap - 1);
}

/*
 * Creates an array of length len with undefined values.
 */
static inline struct aadeque *
aadeque_create(AADEQUE_SIZE_T len) {
	AADEQUE_SIZE_T cap = AADEQUE_MIN_CAPACITY;
	struct aadeque *a;
	while (cap < len)
		cap = cap << 1;
	a = (struct aadeque *)AADEQUE_ALLOC(aadeque_sizeof(cap));
	if (!a) AADEQUE_OOM();
	a->len = len;
	a->off = 0;
	a->cap = cap;
	return a;
}

/*
 * Creates an empty array.
 */
static inline struct aadeque *
aadeque_create_empty(void) {
	return aadeque_create(0);
}

/*
 * Frees the memory.
 */
static inline void
aadeque_destroy(struct aadeque *a) {
	AADEQUE_FREE(a, aadeque_sizeof(a->cap));
}

/*
 * Returns the number of elements in the array.
 */
static inline AADEQUE_SIZE_T
aadeque_len(struct aadeque *a) {
	return a->len;
}

/*
 * Fetch the element at the zero based index i. The index bounds are not
 * checked.
 */
static inline AADEQUE_VALUE_T
aadeque_get(struct aadeque *a, AADEQUE_SIZE_T i) {
	AADEQUE_SIZE_T pos = aadeque_idx(a, i);
	return a->els[pos];
}

/*
 * Set (replace) the element at the zero based index i. The index bounds are not
 * checked.
 */
static inline void
aadeque_set(struct aadeque *a, AADEQUE_SIZE_T i, AADEQUE_VALUE_T value) {
	AADEQUE_SIZE_T pos = aadeque_idx(a, i);
	a->els[pos] = value;
}

/*
 * Clear the memory (set to zery bytes) of n elements at indices between
 * i and i+n-1.
 *
 * Used internally if AADEQUE_CLEAR_UNUSED_MEM is defined.
 */
static inline void
aadeque_clear(struct aadeque *a, AADEQUE_SIZE_T i, AADEQUE_SIZE_T n) {
	if (aadeque_idx(a, i) > aadeque_idx(a, i + n - 1)) {
		/*
		 * It warps. There are two parts to clear.
		 *
		 *     0  i+n-1   i   cap
		 *    /   /      /   /
		 *   |--->      o---|
		 */
		memset(&a->els[0], 0,
		       sizeof(AADEQUE_VALUE_T) * aadeque_idx(a, n + i));
		memset(&a->els[aadeque_idx(a, i)], 0,
		       sizeof(AADEQUE_VALUE_T) * (a->cap - aadeque_idx(a, i)));
	}
	else {
		/*
		 * It's in consecutive memory.
		 *
		 *     0   i   i+n-1  cap
		 *    /   /     /    /
		 *   |   o----->    |
		 */
		memset(&a->els[aadeque_idx(a, i)], 0, sizeof(AADEQUE_VALUE_T) * n);
	}
}

/*
 * Clones an array deque, preserving the internal memory layout.
 */
static inline struct aadeque *
aadeque_clone(struct aadeque *a) {
	size_t sz = aadeque_sizeof(a->cap);
	void *clone = AADEQUE_ALLOC(sz);
	if (!clone) AADEQUE_OOM();
	return (struct aadeque *)memcpy(clone, a, sz);
}

/*
 * Create an struct aadeque from the n values pointed to by array.
 */
static inline struct aadeque *
aadeque_from_array(AADEQUE_VALUE_T *array, AADEQUE_SIZE_T n) {
	struct aadeque *a = aadeque_create(n);
	memcpy(a->els, array, sizeof(AADEQUE_VALUE_T) * n);
	return a;
}

/*
 * Compare the contents of a against a static C array of n elements. Returns 1
 * if the number of elements is equal to n and all elements are equal to their
 * counterparts in array, 0 otherwise.
 */
static inline int
aadeque_eq_array(struct aadeque *a, AADEQUE_VALUE_T *array, AADEQUE_SIZE_T n) {
	AADEQUE_SIZE_T i;
	if (a->len != n)
		return 0;
	for (i = 0; i < n; i++)
		if (aadeque_get(a, i) != array[i])
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
static inline struct aadeque *
aadeque_reserve(struct aadeque *a, AADEQUE_SIZE_T n) {
	if (a->cap < a->len + n) {
		/* calulate and set new capacity */
		AADEQUE_SIZE_T oldcap = a->cap;
		do {
			a->cap = a->cap << 1;
		} while (a->cap < a->len + n);
		/* allocate more mem */
		a = (struct aadeque *)AADEQUE_REALLOC(a,
		                                      aadeque_sizeof(a->cap),
		                                      aadeque_sizeof(oldcap));
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
static inline struct aadeque *
aadeque_compact_to(struct aadeque *a, AADEQUE_SIZE_T mincap) {
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
		a = (struct aadeque *)AADEQUE_REALLOC(a,
		                                      aadeque_sizeof(a->cap),
		                                      aadeque_sizeof(oldcap));
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
static inline struct aadeque *
aadeque_compact_some(struct aadeque *a) {
	return aadeque_compact_to(a, a->len << 1);
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
static inline struct aadeque *
aadeque_make_space_after(struct aadeque *a, AADEQUE_SIZE_T n) {
	a = aadeque_reserve(a, n);
	a->len += n;
	return a;
}

/*
 * Inserts n undefined values before the first element.
 */
static inline struct aadeque *
aadeque_make_space_before(struct aadeque *a, AADEQUE_SIZE_T n) {
	a = aadeque_reserve(a, n);
	a->off = aadeque_idx(a, a->cap - n);
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
static inline struct aadeque *
aadeque_crop(struct aadeque *a, AADEQUE_SIZE_T offset, AADEQUE_SIZE_T length) {
	#ifdef AADEQUE_CLEAR_UNUSED_MEM
	aadeque_clear(a, offset + length, a->len - length);
	#endif
	a->off = aadeque_idx(a, offset);
	a->len = length;
	return aadeque_compact_some(a);
}

/*
 * Deletes the last n elements.
 */
static inline struct aadeque *
aadeque_delete_last_n(struct aadeque *a, AADEQUE_SIZE_T n) {
	return aadeque_crop(a, 0, a->len - n);
}

/*
 * Deletes the first n elements.
 */
static inline struct aadeque *
aadeque_delete_first_n(struct aadeque *a, AADEQUE_SIZE_T n) {
	return aadeque_crop(a, n, a->len - n);
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
aadeque_unshift(struct aadeque **aptr, AADEQUE_VALUE_T value) {
	*aptr = aadeque_make_space_before(*aptr, 1);
	(*aptr)->els[(*aptr)->off] = value;
}

/*
 * Remove an element at the beginning and return its value.
 * May change aptr if it needs to be reallocated.
*/
static inline AADEQUE_VALUE_T
aadeque_shift(struct aadeque **aptr) {
	AADEQUE_VALUE_T value = (*aptr)->els[(*aptr)->off];
	*aptr = aadeque_crop(*aptr, 1, (*aptr)->len - 1);
	return value;
}

/*
 * Insert an element at the end.
 * May change aptr if it needs to be reallocated.
 */
static inline void
aadeque_push(struct aadeque **aptr, AADEQUE_VALUE_T value) {
	*aptr = aadeque_make_space_after(*aptr, 1);
	(*aptr)->els[aadeque_idx(*aptr, (*aptr)->len - 1)] = value;
}

/*
 * Remove an element at the end and return its value.
 * May change aptr if it needs to be reallocated.
 */
static inline AADEQUE_VALUE_T
aadeque_pop(struct aadeque **aptr) {
	AADEQUE_VALUE_T value =
		(*aptr)->els[aadeque_idx(*aptr, (*aptr)->len - 1)];
	*aptr = aadeque_crop(*aptr, 0, (*aptr)->len - 1);
	return value;
}

/*---------------------------------------------------------------------------
 * Append or prepend all elements of one array deque to another
 *---------------------------------------------------------------------------*/

/*
 * Appends all elements of s2 to a1 and returns the (possibly reallocated) a1.
 */
static inline struct aadeque *
aadeque_append(struct aadeque *a1, struct aadeque *a2) {
	AADEQUE_SIZE_T i = a1->len, j;
	a1 = aadeque_make_space_after(a1, a2->len);
	/* TODO: copy using memcpy instead of the loop */
	for (j = 0; j < a2->len; j++)
		aadeque_set(a1, i++, aadeque_get(a2, j));
	return a1;
}

/*
 * Prepends all elements of s2 to a1 and returns the (possibly reallocated) a1.
 */
static inline struct aadeque *
aadeque_prepend(struct aadeque *a1, struct aadeque *a2) {
	AADEQUE_SIZE_T i = 0, j;
	a1 = aadeque_make_space_before(a1, a2->len);
	/* TODO: copy using memcpy instead of the loop */
	for (j = 0; j < a2->len; j++)
		aadeque_set(a1, i++, aadeque_get(a2, j));
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
static inline struct aadeque *
aadeque_slice(struct aadeque *a, AADEQUE_SIZE_T offset, AADEQUE_SIZE_T length) {
	struct aadeque *b = aadeque_create(length);
	/* TODO: use memcpy instead of the loop */
	AADEQUE_SIZE_T i;
	for (i = 0; i < length; i++) {
		AADEQUE_VALUE_T x = aadeque_get(a, i + offset);
		aadeque_set(b, i, x);
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
static inline struct aadeque *
aadeque_compact(struct aadeque *a) {
	return aadeque_compact_to(a, a->len);
}

/*
 * Joins the parts together in memory, possibly in the wrong order. This is
 * useful if you want to sort or shuffle the underlaying array using functions
 * for raw arrays (such as qsort).
 */
static inline void
aadeque_make_contiguous_unordered(struct aadeque *a) {
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
