//
// Test suite for the list module (src/list/list.h).
//
// Test cases: api, edge, ownership, stress, memory.
//

#include "collections_tests.h"
#include "../src/list/list.h"
#include <stdlib.h>

/** JOB CALLBACKS AND HELPERS **/

/* Counts visited elements. */
static int count_job(void *obj, void *argstruct) {
    (void) obj;
    ++*(size_t *) argstruct;
    return 0;
}


/* Collects visited element pointers in order. */
#define COLLECT_MAX 8

typedef struct {
    void  *items[COLLECT_MAX];
    size_t n;
} collect_arg_t;

static int collect_job(void *obj, void *argstruct) {
    collect_arg_t *a = argstruct;
    if (a->n < COLLECT_MAX) a->items[a->n++] = obj;
    return 0;
}


/* Stops the iteration after stop_after elements. */
typedef struct {
    size_t visited;
    size_t stop_after;
} stop_arg_t;

static int stop_job(void *obj, void *argstruct) {
    (void) obj;
    stop_arg_t *a = argstruct;
    a->visited++;
    return a->visited >= a->stop_after ? 1 : 0;
}


/* Counts visited elements and sums the pointed-to int values. */
typedef struct {
    size_t        count;
    unsigned long sum;
} sum_arg_t;

static int sum_job(void *obj, void *argstruct) {
    sum_arg_t *a = argstruct;
    a->count++;
    a->sum += (unsigned long) *(int *) obj;
    return 0;
}


/* Destructor counting its invocations. */
static size_t dtor_calls;

static void counting_dtor(void *obj) {
    (void) obj;
    ++dtor_calls;
}


/** TEST CASES **/

// Basic API behaviour: prepend order, first-occurrence removal,
// size/is_empty out-params, foreach early stop.
static int api(void) {
    int    a, b, c;
    size_t n;
    int    empty;

    list_t *l = list_create(NULL);
    ASSERT(l != NULL);

    ASSERT(list_is_empty(l, &empty) == LIST_OK && empty == 1);
    ASSERT(list_size(l, &n) == LIST_OK && n == 0);

    ASSERT(list_add(l, &a) == LIST_OK);
    ASSERT(list_size(l, &n) == LIST_OK && n == 1);
    ASSERT(list_is_empty(l, &empty) == LIST_OK && empty == 0);
    ASSERT(list_add(l, &b) == LIST_OK);
    ASSERT(list_size(l, &n) == LIST_OK && n == 2);
    ASSERT(list_add(l, &c) == LIST_OK);
    ASSERT(list_size(l, &n) == LIST_OK && n == 3);

    /* add prepends, so foreach visits in reverse insertion order: c, b, a. */
    collect_arg_t col = {.n = 0};
    ASSERT(list_foreach(l, collect_job, &col) == LIST_OK);
    ASSERT(col.n == 3);
    ASSERT(col.items[0] == &c && col.items[1] == &b && col.items[2] == &a);

    /* Insert the same pointer twice; remove drops only the FIRST occurrence. */
    ASSERT(list_add(l, &a) == LIST_OK); /* list: a c b a */
    ASSERT(list_size(l, &n) == LIST_OK && n == 4);
    ASSERT(list_remove(l, &a) == LIST_OK); /* removes the head copy */
    ASSERT(list_size(l, &n) == LIST_OK && n == 3);

    col.n = 0;
    ASSERT(list_foreach(l, collect_job, &col) == LIST_OK);
    ASSERT(col.n == 3);
    ASSERT(col.items[0] == &c && col.items[1] == &b && col.items[2] == &a);
    size_t occurrences = 0;
    for (size_t i = 0; i < col.n; ++i)
        if (col.items[i] == &a) ++occurrences;
    ASSERT(occurrences == 1); /* the second copy survived */

    /* Early stop: job returns non-zero after 2 elements; foreach still
     * returns LIST_OK and exactly 2 elements were visited. */
    stop_arg_t st = {.visited = 0, .stop_after = 2};
    ASSERT(list_foreach(l, stop_job, &st) == LIST_OK);
    ASSERT(st.visited == 2);

    ASSERT(list_is_empty(l, &empty) == LIST_OK && empty == 0);
    ASSERT(list_destroy(l) == LIST_OK);
    return PASS;
}


// Argument validation and boundary conditions.
static int edge(void) {
    size_t n;
    int    flag;
    int    x = 42, y = 7;

    /* Every function with a NULL list reports EINVAL. */
    errno = 0;
    ASSERT(list_destroy(NULL) == LIST_ERR && errno == EINVAL);
    errno = 0;
    ASSERT(list_add(NULL, &x) == LIST_ERR && errno == EINVAL);
    errno = 0;
    ASSERT(list_remove(NULL, &x) == LIST_ERR && errno == EINVAL);
    errno = 0;
    ASSERT(list_size(NULL, &n) == LIST_ERR && errno == EINVAL);
    errno = 0;
    ASSERT(list_is_empty(NULL, &flag) == LIST_ERR && errno == EINVAL);
    errno = 0;
    ASSERT(list_foreach(NULL, count_job, &n) == LIST_ERR && errno == EINVAL);

    list_t *l = list_create(NULL);
    ASSERT(l != NULL);

    /* NULL out-pointers and NULL job report EINVAL. */
    errno = 0;
    ASSERT(list_size(l, NULL) == LIST_ERR && errno == EINVAL);
    errno = 0;
    ASSERT(list_is_empty(l, NULL) == LIST_ERR && errno == EINVAL);
    errno = 0;
    ASSERT(list_foreach(l, NULL, NULL) == LIST_ERR && errno == EINVAL);

    /* Operations on a freshly created empty list. */
    n = 99;
    ASSERT(list_size(l, &n) == LIST_OK && n == 0);
    flag = 0;
    ASSERT(list_is_empty(l, &flag) == LIST_OK && flag == 1);
    size_t cnt = 0;
    ASSERT(list_foreach(l, count_job, &cnt) == LIST_OK && cnt == 0);

    /* Removing from an empty list reports ENOENT. */
    errno = 0;
    ASSERT(list_remove(l, &x) == LIST_ERR && errno == ENOENT);

    /* Removing an absent pointer reports ENOENT and does not alter the list. */
    ASSERT(list_add(l, &x) == LIST_OK);
    errno = 0;
    ASSERT(list_remove(l, &y) == LIST_ERR && errno == ENOENT);
    ASSERT(list_size(l, &n) == LIST_OK && n == 1);
    ASSERT(list_remove(l, &x) == LIST_OK);
    ASSERT(list_size(l, &n) == LIST_OK && n == 0);

    /* Destroying an (again) empty list succeeds. */
    ASSERT(list_destroy(l) == LIST_OK);
    return PASS;
}


// Destructor semantics: exactly one destructor run per disposed element,
// no runs without a destructor.
static int ownership(void) {
    static int elems[5];
    size_t     n;

    dtor_calls = 0;
    list_t *l  = list_create(counting_dtor);
    ASSERT(l != NULL);
    for (size_t i = 0; i < SIZE(elems); ++i)
        ASSERT(list_add(l, &elems[i]) == LIST_OK);
    ASSERT(dtor_calls == 0); /* adding never destroys */

    ASSERT(list_remove(l, &elems[2]) == LIST_OK);
    ASSERT(dtor_calls == 1); /* exactly once per removed element */
    ASSERT(list_remove(l, &elems[0]) == LIST_OK);
    ASSERT(dtor_calls == 2);

    /* A failed remove must not run the destructor. */
    errno = 0;
    ASSERT(list_remove(l, &elems[0]) == LIST_ERR && errno == ENOENT);
    ASSERT(dtor_calls == 2);

    ASSERT(list_size(l, &n) == LIST_OK && n == 3);
    ASSERT(list_destroy(l) == LIST_OK);
    ASSERT(dtor_calls == 5); /* destroy ran it on exactly the 3 remaining */

    /* NULL destructor: the caller keeps ownership, elements stay untouched. */
    l = list_create(NULL);
    ASSERT(l != NULL);
    int *heap1 = malloc(sizeof *heap1);
    ASSERT(heap1 != NULL);
    *heap1     = 123;
    int *heap2 = malloc(sizeof *heap2);
    ASSERT(heap2 != NULL);
    *heap2 = 456;
    ASSERT(list_add(l, heap1) == LIST_OK);
    ASSERT(list_add(l, heap2) == LIST_OK);
    ASSERT(list_remove(l, heap1) == LIST_OK);
    ASSERT(*heap1 == 123); /* not freed, not modified */
    ASSERT(list_destroy(l) == LIST_OK);
    ASSERT(*heap2 == 456); /* not freed, not modified */
    free(heap1);
    free(heap2);
    return PASS;
}


// Many elements and interleaved add/remove rounds exercising the
// node-recycling pool.
#define STRESS_N      10000
#define STRESS_HALF   (STRESS_N / 2)
#define STRESS_ROUNDS 4

static int stress(void) {
    static int vals[STRESS_N];
    size_t     n;

    for (size_t i = 0; i < STRESS_N; ++i) vals[i] = (int) i;

    unsigned long expected_sum = 0;
    for (size_t i = 0; i < STRESS_N; ++i)
        expected_sum += (unsigned long) vals[i];

    list_t *l = list_create(NULL);
    ASSERT(l != NULL);
    for (size_t i = 0; i < STRESS_N; ++i)
        ASSERT(list_add(l, &vals[i]) == LIST_OK);
    ASSERT(list_size(l, &n) == LIST_OK && n == STRESS_N);

    sum_arg_t s = {.count = 0, .sum = 0};
    ASSERT(list_foreach(l, sum_job, &s) == LIST_OK);
    ASSERT(s.count == STRESS_N && s.sum == expected_sum);

    for (int round = 0; round < STRESS_ROUNDS; ++round) {
        /* Remove the front half; those nodes go to the recycling pool. */
        for (size_t i = STRESS_N; i-- > STRESS_HALF;)
            ASSERT(list_remove(l, &vals[i]) == LIST_OK);
        ASSERT(list_size(l, &n) == LIST_OK && n == STRESS_HALF);

        sum_arg_t half = {.count = 0, .sum = 0};
        ASSERT(list_foreach(l, sum_job, &half) == LIST_OK);
        ASSERT(half.count == STRESS_HALF);

        /* Add them back; the pool should be reused transparently. */
        for (size_t i = STRESS_HALF; i < STRESS_N; ++i)
            ASSERT(list_add(l, &vals[i]) == LIST_OK);
        ASSERT(list_size(l, &n) == LIST_OK && n == STRESS_N);

        /* Content integrity: element count and value sum are intact. */
        sum_arg_t full = {.count = 0, .sum = 0};
        ASSERT(list_foreach(l, sum_job, &full) == LIST_OK);
        ASSERT(full.count == STRESS_N && full.sum == expected_sum);
    }

    ASSERT(list_destroy(l) == LIST_OK);
    return PASS;
}


// Allocation-failure scenario: every allocating call gets the
// fail-once-retry treatment; everything is freed on every path.
static unsigned long scenario(void) {
    unsigned long visited = 0;
    list_t       *l;
    int          *e[5] = {NULL, NULL, NULL, NULL, NULL};

    /* 0: list_create allocates the list structure. */
    errno = 0;
    if ((l = list_create(free)) != NULL) visited |= V(1, 0);
    else if (errno == ENOMEM && (l = list_create(free)) != NULL)
        visited |= V(2, 0);
    else return visited | V(4, 0); /* This should not execute. */

    /* 1-8: four heap elements, each malloc'ed and added (node allocation). */
    for (int i = 0; i < 4; ++i) {
        int where_elem = 1 + 2 * i;
        int where_add  = 2 + 2 * i;

        errno = 0;
        if ((e[i] = malloc(sizeof *e[i])) != NULL) visited |= V(1, where_elem);
        else if (errno == ENOMEM && (e[i] = malloc(sizeof *e[i])) != NULL)
            visited |= V(2, where_elem);
        else {
            (void) list_destroy(l);
            return visited | V(4, where_elem); /* This should not execute. */
        }
        *e[i] = i;

        errno = 0;
        if (list_add(l, e[i]) == LIST_OK) visited |= V(1, where_add);
        else if (errno == ENOMEM && list_add(l, e[i]) == LIST_OK)
            visited |= V(2, where_add);
        else {
            free(e[i]);
            (void) list_destroy(l);
            return visited | V(4, where_add); /* This should not execute. */
        }
    }

    /* 9: list_size does not allocate. */
    size_t n = 0;
    if (list_size(l, &n) == LIST_OK && n == 4) visited |= V(1, 9);
    else {
        (void) list_destroy(l);
        return visited | V(4, 9); /* This should not execute. */
    }

    /* 10: list_remove does not allocate; it frees e[1] via the destructor
     * and recycles the node. */
    if (list_remove(l, e[1]) == LIST_OK) visited |= V(1, 10);
    else {
        (void) list_destroy(l);
        return visited | V(4, 10); /* This should not execute. */
    }

    /* 11: a fifth heap element. */
    errno = 0;
    if ((e[4] = malloc(sizeof *e[4])) != NULL) visited |= V(1, 11);
    else if (errno == ENOMEM && (e[4] = malloc(sizeof *e[4])) != NULL)
        visited |= V(2, 11);
    else {
        (void) list_destroy(l);
        return visited | V(4, 11); /* This should not execute. */
    }
    *e[4] = 4;

    /* 12: this add reuses the node recycled in step 10 - no allocation. */
    if (list_add(l, e[4]) == LIST_OK) visited |= V(1, 12);
    else {
        free(e[4]);
        (void) list_destroy(l);
        return visited | V(4, 12); /* This should not execute. */
    }

    /* 13: list_foreach does not allocate. */
    size_t cnt = 0;
    if (list_foreach(l, count_job, &cnt) == LIST_OK && cnt == 4)
        visited |= V(1, 13);
    else {
        (void) list_destroy(l);
        return visited | V(4, 13); /* This should not execute. */
    }

    /* 14: list_is_empty does not allocate. */
    int empty = 1;
    if (list_is_empty(l, &empty) == LIST_OK && empty == 0) visited |= V(1, 14);
    else {
        (void) list_destroy(l);
        return visited | V(4, 14); /* This should not execute. */
    }

    /* 15: destroy frees the 4 remaining elements, all nodes (including the
     * recycling pool) and the list structure. */
    if (list_destroy(l) == LIST_OK) visited |= V(1, 15);
    else return visited | V(4, 15); /* This should not execute. */

    return visited;
}


// Sweeps an injected allocation failure over every allocation site.
static int memory(void) {
    memory_tests_check();
    return memory_test(scenario);
}


/** TEST REGISTRY **/

static const test_list_t test_list[] = {TEST(api), TEST(edge), TEST(ownership),
                                        TEST(stress), TEST(memory)};

int main(int argc, char *argv[]) {
    return run_test_main(argc, argv, test_list, SIZE(test_list));
}
