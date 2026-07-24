//
// Test suite for the stack module (src/stack/stack.{h,c}).
//
// Test cases:
//   api       - core semantics: LIFO order, ownership transfer on pop,
//               size/is_empty tracking, foreach order and early stop;
//   edge      - error reporting: EINVAL for NULL arguments, ENOENT for
//               popping an empty stack, single-element flip-flopping;
//   ownership - destructor discipline: called exactly once per element on
//               pop(NULL) and on destroy, never when ownership transfers;
//   stress    - 10000+ elements, interleaved push/pop waves exercising
//               the node recycling pool, LIFO integrity per wave;
//   memory    - allocation-failure sweep in the style of test/ma_tests.c.
//

#include "collections_tests.h"
#include "../src/stack/stack.h"

#include <stdlib.h>

/* --- Helpers ------------------------------------------------------------ */

/* Allocates an int on the heap; aborts the test process on real OOM. */
static int *make_int(int value) {
    int *p = malloc(sizeof *p);
    assert(p != NULL);
    *p = value;
    return p;
}


/* foreach job recording visited int values in push order of visitation.
 * Returns non-zero (stops iteration) once `limit` elements were seen. */
typedef struct {
    int    values[32];
    size_t count;
    size_t limit;
} visit_log_t;

static int record_job(void *obj, void *argstruct) {
    visit_log_t *log = argstruct;
    assert(log->count < SIZE(log->values));
    log->values[log->count++] = *(int *) obj;
    return log->count >= log->limit ? 1 : 0;
}


/* foreach job summing int values; used by the memory scenario. */
static int sum_job(void *obj, void *argstruct) {
    *(int *) argstruct += *(int *) obj;
    return 0;
}


/* foreach job that never runs anything useful; used for NULL-stack checks. */
static int noop_job(void *obj, void *argstruct) {
    (void) obj;
    (void) argstruct;
    return 0;
}


/* Counting destructor over statically allocated elements. */
static size_t dtor_calls;

static void counting_dtor(void *obj) {
    (void) obj;
    ++dtor_calls;
}


/* --- api ---------------------------------------------------------------- */

static int api(void) {
    enum {
        N = 10
    };

    stack_t *s;
    int     *elems[N];
    size_t   n     = 999;
    int      empty = -1;
    void    *out   = NULL;

    s = stack_create(free);
    ASSERT(s != NULL);

    /* Freshly created stack is empty. */
    ASSERT(stack_size(s, &n) == STACK_OK);
    ASSERT(n == 0);
    ASSERT(stack_is_empty(s, &empty) == STACK_OK);
    ASSERT(empty == 1);

    /* Push N distinct heap values, tracking size/is_empty along the way. */
    for (size_t i = 0; i < N; ++i) {
        elems[i] = make_int((int) (100 + i));
        ASSERT(stack_push(s, elems[i]) == STACK_OK);
        ASSERT(stack_size(s, &n) == STACK_OK);
        ASSERT(n == i + 1);
        ASSERT(stack_is_empty(s, &empty) == STACK_OK);
        ASSERT(empty == 0);
    }

    /* foreach visits from the top down: last pushed first. */
    visit_log_t log = {.count = 0, .limit = SIZE(log.values)};
    ASSERT(stack_foreach(s, record_job, &log) == STACK_OK);
    ASSERT(log.count == N);
    for (size_t i = 0; i < N; ++i) ASSERT(log.values[i] == *elems[N - 1 - i]);

    /* foreach early stop: non-zero job result stops after exactly k visits
     * and the call still reports STACK_OK. */
    visit_log_t short_log = {.count = 0, .limit = 3};
    ASSERT(stack_foreach(s, record_job, &short_log) == STACK_OK);
    ASSERT(short_log.count == 3);
    for (size_t i = 0; i < 3; ++i)
        ASSERT(short_log.values[i] == *elems[N - 1 - i]);

    /* Pop everything: strict LIFO, ownership transferred to the caller. */
    for (size_t i = 0; i < N; ++i) {
        out = NULL;
        ASSERT(stack_pop(s, &out) == STACK_OK);
        ASSERT(out == elems[N - 1 - i]);
        ASSERT(*(int *) out == (int) (100 + (N - 1 - i)));
        free(out);
        ASSERT(stack_size(s, &n) == STACK_OK);
        ASSERT(n == N - 1 - i);
    }
    ASSERT(stack_is_empty(s, &empty) == STACK_OK);
    ASSERT(empty == 1);

    /* Push after a full drain still works (recycled nodes). */
    ASSERT(stack_push(s, make_int(7)) == STACK_OK);
    ASSERT(stack_push(s, make_int(8)) == STACK_OK);
    ASSERT(stack_size(s, &n) == STACK_OK);
    ASSERT(n == 2);
    out = NULL;
    ASSERT(stack_pop(s, &out) == STACK_OK);
    ASSERT(out != NULL && *(int *) out == 8);
    free(out);

    /* Pop with NULL out_data destroys the element via the destructor. */
    ASSERT(stack_push(s, make_int(9)) == STACK_OK);
    ASSERT(stack_pop(s, NULL) == STACK_OK);
    ASSERT(stack_size(s, &n) == STACK_OK);
    ASSERT(n == 1);

    /* Destroy with a remaining element; the destructor (free) reclaims it. */
    ASSERT(stack_destroy(s) == STACK_OK);
    return PASS;
}


/* --- edge --------------------------------------------------------------- */

static int edge(void) {
    static int a, b;
    stack_t   *s;
    size_t     n     = 999;
    int        empty = -1;
    void      *out   = NULL;

    /* Every function must reject a NULL stack with STACK_ERR / EINVAL. */
    errno = 0;
    ASSERT(stack_destroy(NULL) == STACK_ERR);
    ASSERT(errno == EINVAL);

    errno = 0;
    ASSERT(stack_push(NULL, &a) == STACK_ERR);
    ASSERT(errno == EINVAL);

    errno = 0;
    ASSERT(stack_pop(NULL, &out) == STACK_ERR);
    ASSERT(errno == EINVAL);

    errno = 0;
    ASSERT(stack_pop(NULL, NULL) == STACK_ERR);
    ASSERT(errno == EINVAL);

    errno = 0;
    ASSERT(stack_size(NULL, &n) == STACK_ERR);
    ASSERT(errno == EINVAL);
    ASSERT(n == 999);

    errno = 0;
    ASSERT(stack_is_empty(NULL, &empty) == STACK_ERR);
    ASSERT(errno == EINVAL);
    ASSERT(empty == -1);

    errno = 0;
    ASSERT(stack_foreach(NULL, noop_job, NULL) == STACK_ERR);
    ASSERT(errno == EINVAL);

    s = stack_create(NULL);
    ASSERT(s != NULL);

    /* NULL out-pointers and a NULL job are rejected with EINVAL. */
    errno = 0;
    ASSERT(stack_size(s, NULL) == STACK_ERR);
    ASSERT(errno == EINVAL);

    errno = 0;
    ASSERT(stack_is_empty(s, NULL) == STACK_ERR);
    ASSERT(errno == EINVAL);

    errno = 0;
    ASSERT(stack_foreach(s, NULL, NULL) == STACK_ERR);
    ASSERT(errno == EINVAL);

    /* Popping an empty stack reports ENOENT, with and without out_data. */
    errno = 0;
    ASSERT(stack_pop(s, &out) == STACK_ERR);
    ASSERT(errno == ENOENT);
    ASSERT(out == NULL);

    errno = 0;
    ASSERT(stack_pop(s, NULL) == STACK_ERR);
    ASSERT(errno == ENOENT);

    /* Alternating single push/pop: the size flips between 0 and 1. */
    for (size_t i = 0; i < 100; ++i) {
        int *expected = (i % 2 == 0) ? &a : &b;
        ASSERT(stack_push(s, expected) == STACK_OK);
        ASSERT(stack_size(s, &n) == STACK_OK);
        ASSERT(n == 1);
        ASSERT(stack_is_empty(s, &empty) == STACK_OK);
        ASSERT(empty == 0);
        out = NULL;
        ASSERT(stack_pop(s, &out) == STACK_OK);
        ASSERT(out == expected);
        ASSERT(stack_size(s, &n) == STACK_OK);
        ASSERT(n == 0);
        ASSERT(stack_is_empty(s, &empty) == STACK_OK);
        ASSERT(empty == 1);
        /* The drained stack keeps reporting ENOENT. */
        errno = 0;
        ASSERT(stack_pop(s, &out) == STACK_ERR);
        ASSERT(errno == ENOENT);
    }

    ASSERT(stack_destroy(s) == STACK_OK);
    return PASS;
}


/* --- ownership ---------------------------------------------------------- */

static int ownership(void) {
    enum {
        N = 5
    };

    static int st_vals[N];
    stack_t   *s;
    void      *out = NULL;

    dtor_calls = 0;
    s          = stack_create(counting_dtor);
    ASSERT(s != NULL);

    /* pop(NULL) runs the destructor exactly once per element. */
    for (size_t i = 0; i < N; ++i)
        ASSERT(stack_push(s, &st_vals[i]) == STACK_OK);
    ASSERT(dtor_calls == 0);
    for (size_t i = 0; i < N; ++i) {
        ASSERT(stack_pop(s, NULL) == STACK_OK);
        ASSERT(dtor_calls == i + 1);
    }

    /* Destroying a stack with remaining elements destroys each exactly once. */
    dtor_calls = 0;
    for (size_t i = 0; i < N; ++i)
        ASSERT(stack_push(s, &st_vals[i]) == STACK_OK);
    ASSERT(stack_destroy(s) == STACK_OK);
    ASSERT(dtor_calls == N);

    /* Ownership transfer: the destructor is NOT run when out_data is given. */
    dtor_calls = 0;
    s          = stack_create(counting_dtor);
    ASSERT(s != NULL);
    ASSERT(stack_push(s, &st_vals[2]) == STACK_OK);
    ASSERT(stack_pop(s, &out) == STACK_OK);
    ASSERT(out == &st_vals[2]);
    ASSERT(dtor_calls == 0);

    /* A pushed NULL element does not invoke the destructor. */
    ASSERT(stack_push(s, NULL) == STACK_OK);
    ASSERT(stack_pop(s, NULL) == STACK_OK);
    ASSERT(dtor_calls == 0);
    ASSERT(stack_destroy(s) == STACK_OK);
    ASSERT(dtor_calls == 0);

    /* NULL destructor: pop(NULL) and destroy-with-elements are safe no-ops
     * ownership-wise; the elements are statically allocated. */
    s = stack_create(NULL);
    ASSERT(s != NULL);
    for (size_t i = 0; i < N; ++i)
        ASSERT(stack_push(s, &st_vals[i]) == STACK_OK);
    ASSERT(stack_pop(s, NULL) == STACK_OK);
    ASSERT(stack_pop(s, NULL) == STACK_OK);
    ASSERT(stack_destroy(s) == STACK_OK);

    return PASS;
}


/* --- stress ------------------------------------------------------------- */

#define STRESS_N 10000

static int stress(void) {
    /* Waves of pushes and pops; cumulative counts balance out to zero and
     * repeatedly shrink/regrow the stack so popped nodes get recycled. */
    static size_t const wave_push[] = {STRESS_N, 3000, 5000, 2500};
    static size_t const wave_pop[]  = {4000, 6000, 4500, 6000};

    int    *values = malloc(STRESS_N * sizeof *values);
    size_t *ref    = malloc((STRESS_N + 1) * sizeof *ref); /* shadow stack */
    assert(values != NULL && ref != NULL);
    for (size_t i = 0; i < STRESS_N; ++i) values[i] = (int) i;

    stack_t *s = stack_create(NULL);
    ASSERT(s != NULL);

    size_t ref_top = 0, next = 0, n = 0;
    for (size_t w = 0; w < SIZE(wave_push); ++w) {
        for (size_t i = 0; i < wave_push[w]; ++i) {
            size_t idx = next++ % STRESS_N;
            ASSERT(stack_push(s, &values[idx]) == STACK_OK);
            ref[ref_top++] = idx;
        }
        ASSERT(stack_size(s, &n) == STACK_OK);
        ASSERT(n == ref_top);

        for (size_t i = 0; i < wave_pop[w]; ++i) {
            void *out = NULL;
            ASSERT(stack_pop(s, &out) == STACK_OK);
            size_t idx = ref[--ref_top];
            ASSERT(out == &values[idx]);
            ASSERT(*(int *) out == (int) idx);
        }
        ASSERT(stack_size(s, &n) == STACK_OK);
        ASSERT(n == ref_top);
    }

    ASSERT(ref_top == 0);
    int empty = 0;
    ASSERT(stack_is_empty(s, &empty) == STACK_OK);
    ASSERT(empty == 1);

    ASSERT(stack_destroy(s) == STACK_OK);
    free(values);
    free(ref);
    return PASS;
}


/* --- memory ------------------------------------------------------------- */

/* Allocation-failure scenario in the style of test/ma_tests.c: every
 * allocating call is retried once after an injected ENOMEM; non-allocating
 * calls are checked plainly. All memory is released on every path. */
static unsigned long scenario(void) {
    unsigned long visited = 0;
    stack_t      *s;
    int          *e[4];
    int          *late  = NULL;
    void         *out   = NULL;
    size_t        n     = 0;
    int           empty = -1, sum = 0;

    errno = 0;
    if ((s = stack_create(free)) != NULL) visited |= V(1, 0);
    else if (errno == ENOMEM && (s = stack_create(free)) != NULL)
        visited |= V(2, 0);
    else return visited | V(4, 0); // To nie powinno się wykonać.

    for (size_t i = 0; i < SIZE(e); ++i) {
        unsigned alloc_where = (unsigned) (1 + 2 * i);
        unsigned push_where  = (unsigned) (2 + 2 * i);

        errno = 0;
        if ((e[i] = malloc(sizeof *e[i])) != NULL) visited |= V(1, alloc_where);
        else if (errno == ENOMEM && (e[i] = malloc(sizeof *e[i])) != NULL)
            visited |= V(2, alloc_where);
        else {
            stack_destroy(s);
            return visited | V(4, alloc_where); // To nie powinno się wykonać.
        }
        *e[i] = 1 << i;

        errno = 0;
        if (stack_push(s, e[i]) == STACK_OK) visited |= V(1, push_where);
        else if (errno == ENOMEM && stack_push(s, e[i]) == STACK_OK)
            visited |= V(2, push_where);
        else {
            free(e[i]);
            stack_destroy(s);
            return visited | V(4, push_where); // To nie powinno się wykonać.
        }
    }

    /* Non-allocating calls must succeed regardless of injected failures. */
    assert(stack_size(s, &n) == STACK_OK && n == 4);
    assert(stack_is_empty(s, &empty) == STACK_OK && empty == 0);
    assert(stack_foreach(s, sum_job, &sum) == STACK_OK);
    assert(sum == 1 + 2 + 4 + 8);

    assert(stack_pop(s, &out) == STACK_OK); /* takes ownership of e[3] */
    assert(out == e[3]);
    free(out);
    assert(stack_pop(s, NULL) == STACK_OK); /* destroys e[2] via free */

    errno = 0;
    if ((late = malloc(sizeof *late)) != NULL) visited |= V(1, 9);
    else if (errno == ENOMEM && (late = malloc(sizeof *late)) != NULL)
        visited |= V(2, 9);
    else {
        stack_destroy(s);
        return visited | V(4, 9); // To nie powinno się wykonać.
    }
    *late = 16;

    /* This push reuses a recycled node, but keep the retry pattern anyway. */
    errno = 0;
    if (stack_push(s, late) == STACK_OK) visited |= V(1, 10);
    else if (errno == ENOMEM && stack_push(s, late) == STACK_OK)
        visited |= V(2, 10);
    else {
        free(late);
        stack_destroy(s);
        return visited | V(4, 10); // To nie powinno się wykonać.
    }

    assert(stack_size(s, &n) == STACK_OK && n == 3);

    /* Destroys the remaining elements (late, e[1], e[0]) with free. */
    assert(stack_destroy(s) == STACK_OK);

    return visited;
}


static int memory(void) {
    memory_tests_check();
    return memory_test(scenario);
}


/* --- Registry ----------------------------------------------------------- */

static const test_list_t test_list[] = {
        TEST(api), TEST(edge), TEST(ownership), TEST(stress), TEST(memory),
};

int main(int argc, char *argv[]) {
    return run_test_main(argc, argv, test_list, SIZE(test_list));
}
