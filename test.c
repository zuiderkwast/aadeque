/*
 * Tests for aadeque.h
 */
#include <stdlib.h>

/* defining tweaking macros, before including aadeque.h */

#define AADEQUE_VALUE_TYPE int

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

int main() {
	int values  [5] = {1, 2, 3, 4, 5};
	int expected[5] = {5, 3, 1, 2, 4};
	int reversed[5];
	int i, ok;
	aadeque_t *a;

	/* build array by alternate pushing and unshifting values in both ends */
	a = aadeque_create_empty();
	for (i = 0; i < 5; i++) {
		if (i % 2) aadeque_push(&a, values[i]);
		else aadeque_unshift(&a, values[i]);
	}

	/* check contents against expecteded values */
	ok = 1;
	for (i = 0; i < 5; i++) ok &= aadeque_get(a, i) == expected[i];
	test(ok, "Alternate push and unshift expecteded contents");

	test(aadeque_len(a) == 5, "Length as expecteded");

	test(allocated_bytes == sizeof(aadeque_t) + sizeof(int) * (8 - 1),
	     "Allocated bytes as expecteded");

	test(num_allocations == 2,
	     "Allocation occured 2 times so far as expecteded");

	/* create reversed array by popping and shifting */
	for (i = 0; i < 5; i++)
		reversed[i] = i % 2 ? aadeque_pop(&a) : aadeque_shift(&a);

	/* check reversed contents */
	ok = 1;
	for (i = 0; i < 5; i++) ok &= reversed[i] == 5 - i;
	test(ok, "Popping and shifting returned expecteded values");

	test(num_allocations == 3,
	     "Allocation occured 3 times so far as expecteded");

	/* destroy and check that all memory was freed */
	aadeque_destroy(a);
	test(allocated_bytes == 0, "Destroy frees all allocated memory");	

	return 0;
}

