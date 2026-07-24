//
// Shared harness for the c-collections test suite.
//
// Test binaries take a single argument - the test case name - and return
// PASS (0), FAIL (1) or WRONG_TEST (2). Memory tests: a scenario function
// reports each step's outcome in a visited-points bitmap and memory_test()
// sweeps an injected allocation failure over every allocation site
// (see test/memory_tests.h).
//

#ifndef COLLECTIONS_TESTS_H
#define COLLECTIONS_TESTS_H

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "memory_tests.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Possible test results */
#define PASS       0
#define FAIL       1
#define WRONG_TEST 2

/* Number of elements of array x */
#define SIZE(x) (sizeof x / sizeof x[0])

#define ASSERT(f)                                                              \
    do {                                                                       \
        if (!(f)) return FAIL;                                                 \
    } while (0)

/* Records the outcome of scenario step `where` in a visited-points bitmap:
 *   1 - the call succeeded normally,
 *   2 - the call failed with ENOMEM and succeeded when retried,
 *   4 - unexpected behaviour (fails the memory test). */
#define V(code, where) (((unsigned long) (code)) << (3 * (where)))

/* Runs an allocation-failure scenario repeatedly, failing the 1st, 2nd, ...
 * allocation in turn, until three consecutive runs see no injected failure.
 * A run fails if the scenario leaked memory (alloc_counter != free_counter)
 * or reported an unexpected outcome (a 4 digit in the bitmap). */
static int memory_test(unsigned long (*test_function)(void)) {
    memory_test_data_t *mtd = get_memory_test_data();

    unsigned fail = 0, pass = 0;
    mtd->call_total   = 0;
    mtd->fail_counter = 1;
    while (fail < 3 && pass < 3) {
        mtd->call_counter            = 0;
        mtd->alloc_counter           = 0;
        mtd->free_counter            = 0;
        mtd->function_name           = NULL;
        unsigned long visited_points = test_function();
        if (mtd->alloc_counter != mtd->free_counter ||
            (visited_points & 0444444444444444444444UL) != 0) {
            fprintf(stderr,
                    "fail_counter %u, alloc_counter %u, free_counter %u, "
                    "function_name %s, visited_point %lo\n",
                    mtd->fail_counter, mtd->alloc_counter, mtd->free_counter,
                    (char const *) mtd->function_name, visited_points);
            ++fail;
        }
        if (mtd->function_name == NULL) ++pass;
        else pass = 0;
        mtd->fail_counter++;
    }

    return mtd->call_total > 0 && fail == 0 ? PASS : FAIL;
}


/* Test registry */
typedef struct {
    char const *name;
    int (*function)(void);
} test_list_t;

#define TEST(t)                                                                \
    { #t, t }

/* Printed after a test case ran to completion (i.e. did not crash);
 * checked by the Python orchestrator in addition to the exit code. */
#define TEST_COMPLETE_LINE "collections-test-complete"

static int run_test_main(int argc, char *argv[], test_list_t const *list,
                         size_t n) {
    if (argc == 2) {
        for (size_t i = 0; i < n; ++i) {
            if (strcmp(argv[1], list[i].name) == 0) {
                int result = list[i].function();
                puts(TEST_COMPLETE_LINE);
                return result;
            }
        }
    }
    fprintf(stderr, "Usage:\n%s test_name\n", argv[0]);
    return WRONG_TEST;
}


#endif // COLLECTIONS_TESTS_H
