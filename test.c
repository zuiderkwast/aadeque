/*
 * Tests for aadeque.h
 */
#include <stdlib.h>

/* defining tweaking macros, before including aadeque.h */
#define AADEQUE_VALUE_TYPE int
#define AADEQUE_MIN_CAPACITY 4

/* tweak allocation, to keep track allocated bytes */
#define AADEQUE_ALLOC(size) test_alloc(size)
#define AADEQUE_REALLOC(ptr, size, oldsize) test_realloc(ptr, size, oldsize)
#define AADEQUE_FREE(ptr, size) test_free(ptr, size)

static size_t allocated_bytes = 0;
static int    num_allocations = 0;

void *test_alloc(size_t size) {
	allocated_bytes += size;
	num_allocations++;
	return malloc(size);
}

void *test_realloc(void *ptr, size_t size, size_t old_size) {
	allocated_bytes += size - old_size;
	num_allocations++;
	return realloc(ptr, size);
}

void test_free(void *ptr, size_t size) {
	allocated_bytes -= size;
	free(ptr);
}


#include "aadeque.h"

#include <stdio.h>

void test(int cond, const char * msg) {
	if (cond) printf("%-70s [ OK ]\n", msg);
	else      printf("%-70s [FAIL]\n", msg);
}

/* debug helper: print the aadeque in a readable format */
static inline void dump(aadeque_t *a) {
	int i;
	printf("[");
	for (i = 0; i < a->len; i++) {
		if (i) printf(",");
		printf("%d", aadeque_get(a, i));
	}
	printf("]\n");
}

void test_push(void) {
	/* Build deque from empty using push and compare. */
	int values[5] = {1, 2, 3, 4, 5};
	aadeque_t *a = aadeque_create_empty();
	int i;
	for (i = 0; i < 5; i++)
		aadeque_push(&a, values[i]);
	test(aadeque_eq_array(a, values, 5), "aadeque_push");
	aadeque_destroy(a);
}

void test_pop(void) {
	/* Reduce deque, checking that we receive the elements in reverse order. */
	int values[5] = {1, 2, 3, 4, 5};
	aadeque_t *a = aadeque_from_array(values, 5);
	int i, ok = 1;
	for (i = 4; i >= 0; i--)
		ok &= aadeque_pop(&a) == values[i];
	test(ok, "aadeque_pop");
	aadeque_destroy(a);
}

void test_unshift(void) {
	/* Build deque from empty by unshifting, iterating values backwards. */
	int values[5] = {1, 2, 3, 4, 5};
	aadeque_t *a = aadeque_create_empty();
	int i;
	for (i = 4; i >= 0; i--)
		aadeque_unshift(&a, values[i]);
	test(aadeque_eq_array(a, values, 5), "aadeque_unshift");
	aadeque_destroy(a);
}

void test_shift(void) {
	/* Reduce deque, checking that we receive the elements in order. */
	int values[5] = {1, 2, 3, 4, 5};
	aadeque_t *a = aadeque_from_array(values, 5);
	int i, ok = 1;
	for (i = 0; i < 5; i++)
		ok &= aadeque_shift(&a) == values[i];
	test(ok, "aadeque_shift");
	aadeque_destroy(a);
}

void test_append(void) {
	int data1 [3] = {1, 2, 3},
		data2 [2] = {4, 5},
		expect[5] = {1, 2, 3, 4, 5};
	aadeque_t *a1 = aadeque_from_array(data1, 3),
	          *a2 = aadeque_from_array(data2, 2);
	a1 = aadeque_append(a1, a2);
	test(aadeque_eq_array(a1, expect, 5), "aadeque_append: result");
	test(aadeque_eq_array(a2, data2, 2), "aadeque_append: untouched operand");
	aadeque_destroy(a1);
	aadeque_destroy(a2);
}

void test_prepend(void) {
	int data1 [3] = {1, 2, 3},
		data2 [2] = {4, 5},
		expect[5] = {4, 5, 1, 2, 3};
	aadeque_t *a1 = aadeque_from_array(data1, 3),
	          *a2 = aadeque_from_array(data2, 2);
	a1 = aadeque_prepend(a1, a2);
	test(aadeque_eq_array(a1, expect, 5), "aadeque_prepend: result");
	test(aadeque_eq_array(a2, data2, 2), "aadeque_prepend: untouched operand");
	aadeque_destroy(a1);
	aadeque_destroy(a2);
}

void test_delete_last_n(void) {
	int before[5] = {1, 2, 3, 4, 5},
	    after [2] = {1, 2};
	aadeque_t *a = aadeque_from_array(before, 5);
	a = aadeque_delete_last_n(a, 3);
	test(aadeque_eq_array(a, after, 2), "aadeque_delete_last_n");
	aadeque_destroy(a);
}

void test_delete_first_n(void) {
	int before[5] = {1, 2, 3, 4, 5},
	    after [2] = {4, 5};
	aadeque_t *a = aadeque_from_array(before, 5);
	a = aadeque_delete_first_n(a, 3);
	test(aadeque_eq_array(a, after, 2), "aadeque_delete_first_n");
	aadeque_destroy(a);
}

void test_slice(void) {
	int before[7] = {1, 2, 3, 4, 5, 6, 7},
	    after [4] = {3, 4, 5, 6};
	aadeque_t *a = aadeque_from_array(before, 7),
	          *b = aadeque_slice(a, 2, 4);
	test(aadeque_eq_array(b, after, 4), "aadeque_slice");
	aadeque_destroy(a);
	aadeque_destroy(b);
}

/*
 * Growing the memory for special case of memory layout. See the comments in the
 * source code of aadeque_reserve() in aadeque.h.
 */
void test_grow_warping(void) {
	int values[5] = {1, 2, 3, 4, 5};
	/* create an array deque with warping memory layout */
	aadeque_t *a = aadeque_create_empty();
	aadeque_push(&a, 4);
	aadeque_push(&a, 5);
	aadeque_unshift(&a, 3);
	aadeque_unshift(&a, 2);
	aadeque_unshift(&a, 1);
	/* check that it is actually warped */
	test(a->off + a->len > a->cap, "Growing warped memory: setup");
	test(aadeque_eq_array(a, values, 5), "Growing warped memory: contents");
	aadeque_destroy(a);
}

/*
 * Shrinking memory for a special case of memory layout. See the comments in the
 * source code of aadeque_compact_to() in aadeque.h.
 */
void test_shrink_case_1(void) {
	int expected[3] = {1, 2, 3};
	int init    [4] = {2, 3, 4, 5};
	/* create an array deque with warping memory layout */
	aadeque_t *a = aadeque_from_array(init, 4);
	aadeque_unshift(&a, 1);
	/* delete some, but not enough to make it shrink automatically */
	a = aadeque_delete_last_n(a, 2);
	/* check the internal structure */
	test(a->cap == 8 && a->len == 3 && a->off == 7,
	     "Shrinking memory case 1: setup");
	/* compact and check again */
	a = aadeque_compact(a);
	test(a->cap == 4 && a->len == 3 && a->off == 3,
	     "Shrinking memory case 1: compact");
	test(aadeque_eq_array(a, expected, 3),
	     "Shrinking memory case 1: contents");
	aadeque_destroy(a);
}

/*
 * Shrinking memory for a special case of memory layout. See the comments in the
 * source code of aadeque_compact_to() in aadeque.h.
 */
void test_shrink_case_2(void) {
	int expected[3] = {6, 7, 8};
	int init    [8] = {1, 2, 3, 4, 5, 6, 7, 8};
	/* create an array deque that resides entirely in the right half */
	aadeque_t *a = aadeque_from_array(init, 8);
	/* delete some, but not enough to make it shrink automatically */
	a = aadeque_delete_first_n(a, 5);
	/* check the internal structure: totally in the 2nd half, not warped */
	test(a->cap == 8 && a->len == 3 && a->off == 5 && a->off + a->len <= a->cap,
	     "Shrinking memory case 2: setup");
	/* compact and check again */
	a = aadeque_compact(a);
	test(a->cap == 4 && a->len == 3 && a->off == 0,
	     "Shrinking memory case 2: compact");
	test(aadeque_eq_array(a, expected, 3),
	     "Shrinking memory case 2: contents");
	aadeque_destroy(a);
}

/*
 * Shrinking memory for a special case of memory layout. See the comments in the
 * source code of aadeque_compact_to() in aadeque.h.
 */
void test_shrink_case_3(void) {
	int expected[3] = {3, 4, 5};
	int init    [5] = {1, 2, 3, 4, 5};
	/*
	 * Create an array deque that crosses the middle. Not warped before shrink
	 * but must be warped after shrink.
	 */
	aadeque_t *a = aadeque_from_array(init, 5);
	/* delete some, but not enough to make it shrink automatically */
	a = aadeque_delete_first_n(a, 2);
	/*
	 * check the internal structure: crossing the middle, not totally in the 2nd
	 * half, not warped
	 */
	test(a->cap == 8 && a->len == 3 && a->off == 2,
	     "Shrinking memory case 3: setup");
	/* compact and check again */
	a = aadeque_compact(a);
	/* check that it's warped after compacting */
	test(a->cap == 4 && a->len == 3 && a->off == 2,
	     "Shrinking memory case 3: compact");
	test(aadeque_eq_array(a, expected, 3),
	     "Shrinking memory case 3: contents");
	aadeque_destroy(a);
}

void test_memory_clean(void) {
	test(allocated_bytes == 0, "All allocated memory free'd");
}

int main() {
	test_push();
	test_pop();
	test_unshift();
	test_shift();
	test_append();
	test_prepend();
	test_delete_last_n();
	test_delete_first_n();
	test_slice();
	test_grow_warping();
	test_shrink_case_1();
	test_shrink_case_2();
	test_shrink_case_3();
	test_memory_clean();
	return 0;
}
