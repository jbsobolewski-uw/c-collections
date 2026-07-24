//
// Test suite for the queue module (src/queue/queue.h).
//
// Test cases:
//   api       - core FIFO behaviour, ownership transfer, foreach, tail reset
//   edge      - argument validation (EINVAL), empty-queue dequeue (ENOENT),
//               alternating single enqueue/dequeue
//   ownership - destructor invocation counting, NULL-destructor semantics
//   stress    - interleaved enqueue/dequeue waves over the node-recycling pool
//   memory    - allocation-failure sweep (see test/memory_tests.h)
//

#include "collections_tests.h"
#include "../src/queue/queue.h"

#include <stdint.h>
#include <stdlib.h>

/* --- Helpers --- */

/* Number of elements used by the api test. */
#define API_N 64

/* Counts destructor invocations across a single test case. */
static size_t dtor_calls;

static void counting_free(void *p) {
    ++dtor_calls;
    free(p);
}


/* Allocates an int on the heap; aborts the process on real OOM. */
static int *make_int(int v) {
    int *p = (int *) malloc(sizeof *p);
    assert(p != NULL);
    *p = v;
    return p;
}


/* foreach context: records visited ints in order, optionally stopping. */
typedef struct {
    int    values[API_N];
    size_t count;
    size_t stop_after; /* 0 means never stop */
} walk_ctx_t;

static int record_job(void *obj, void *argstruct) {
    walk_ctx_t *ctx           = (walk_ctx_t *) argstruct;
    ctx->values[ctx->count++] = *(int *) obj;
    if (ctx->stop_after != 0 && ctx->count >= ctx->stop_after) return 1;
    return 0;
}


/* foreach job that only counts the visited elements. */
static int count_job(void *obj, void *argstruct) {
    (void) obj;
    ++*(size_t *) argstruct;
    return 0;
}


/* Expects `call` to return QUEUE_ERR with errno == e. */
#define TEST_ERRNO(call, e)                                                    \
    do {                                                                       \
        errno = 0;                                                             \
        ASSERT((call) == QUEUE_ERR);                                           \
        ASSERT(errno == (e));                                                  \
    } while (0)

/* --- Test cases --- */

// Core API: create/destroy, strict FIFO order, ownership transfer on
// dequeue, destructor on dequeue(NULL), size/is_empty tracking, foreach
// in order and with early stop, and the tail-reset path after a full drain.
static int api(void) {
    size_t n     = 12345;
    int    empty = -1;

    dtor_calls = 0;
    queue_t *q = queue_create(counting_free);
    ASSERT(q != NULL);

    ASSERT(queue_size(q, &n) == QUEUE_OK);
    ASSERT(n == 0);
    ASSERT(queue_is_empty(q, &empty) == QUEUE_OK);
    ASSERT(empty == 1);

    /* Enqueue API_N distinct heap values, tracking size and emptiness. */
    for (size_t i = 0; i < API_N; ++i) {
        ASSERT(queue_enqueue(q, make_int((int) i)) == QUEUE_OK);
        ASSERT(queue_size(q, &n) == QUEUE_OK);
        ASSERT(n == i + 1);
        ASSERT(queue_is_empty(q, &empty) == QUEUE_OK);
        ASSERT(empty == 0);
    }

    /* foreach visits front-to-back in FIFO order. */
    walk_ctx_t ctx = {.count = 0, .stop_after = 0};
    ASSERT(queue_foreach(q, record_job, &ctx) == QUEUE_OK);
    ASSERT(ctx.count == API_N);
    for (size_t i = 0; i < API_N; ++i) ASSERT(ctx.values[i] == (int) i);

    /* foreach early stop: non-zero job return stops the walk, still QUEUE_OK,
     * exactly stop_after elements visited. */
    ctx.count      = 0;
    ctx.stop_after = 7;
    ASSERT(queue_foreach(q, record_job, &ctx) == QUEUE_OK);
    ASSERT(ctx.count == 7);
    for (size_t i = 0; i < 7; ++i) ASSERT(ctx.values[i] == (int) i);

    /* Dequeue with non-NULL out_data transfers ownership: the destructor must
     * not run and the caller frees the element. Order must be exact FIFO. */
    for (size_t i = 0; i < API_N / 2; ++i) {
        int *out = NULL;
        ASSERT(queue_dequeue(q, (void **) &out) == QUEUE_OK);
        ASSERT(out != NULL);
        ASSERT(*out == (int) i);
        free(out);
        ASSERT(queue_size(q, &n) == QUEUE_OK);
        ASSERT(n == API_N - i - 1);
    }
    ASSERT(dtor_calls == 0);

    /* Dequeue with NULL out_data destroys the element via the destructor. */
    for (size_t i = API_N / 2; i < API_N; ++i) {
        ASSERT(queue_dequeue(q, NULL) == QUEUE_OK);
        ASSERT(dtor_calls == i - API_N / 2 + 1);
    }
    ASSERT(queue_is_empty(q, &empty) == QUEUE_OK);
    ASSERT(empty == 1);
    ASSERT(queue_size(q, &n) == QUEUE_OK);
    ASSERT(n == 0);

    /* Tail-reset path: after a complete drain the queue must accept new
     * elements and preserve FIFO order again. */
    for (int i = 100; i < 108; ++i)
        ASSERT(queue_enqueue(q, make_int(i)) == QUEUE_OK);
    ASSERT(queue_size(q, &n) == QUEUE_OK);
    ASSERT(n == 8);
    for (int i = 100; i < 108; ++i) {
        int *out = NULL;
        ASSERT(queue_dequeue(q, (void **) &out) == QUEUE_OK);
        ASSERT(out != NULL);
        ASSERT(*out == i);
        free(out);
    }

    ASSERT(queue_destroy(q) == QUEUE_OK);
    return PASS;
}


// Argument validation and empty-queue behaviour.
static int edge(void) {
    void  *out   = NULL;
    size_t n     = 0;
    int    empty = 0;
    int    value = 42;

    /* Every function with a NULL queue: QUEUE_ERR, errno == EINVAL. */
    TEST_ERRNO(queue_destroy(NULL), EINVAL);
    TEST_ERRNO(queue_enqueue(NULL, &value), EINVAL);
    TEST_ERRNO(queue_enqueue(NULL, NULL), EINVAL);
    TEST_ERRNO(queue_dequeue(NULL, &out), EINVAL);
    TEST_ERRNO(queue_dequeue(NULL, NULL), EINVAL);
    TEST_ERRNO(queue_size(NULL, &n), EINVAL);
    TEST_ERRNO(queue_is_empty(NULL, &empty), EINVAL);
    TEST_ERRNO(queue_foreach(NULL, count_job, &n), EINVAL);

    queue_t *q = queue_create(NULL);
    ASSERT(q != NULL);

    /* NULL out-pointers and NULL job. */
    TEST_ERRNO(queue_size(q, NULL), EINVAL);
    TEST_ERRNO(queue_is_empty(q, NULL), EINVAL);
    TEST_ERRNO(queue_foreach(q, NULL, &value), EINVAL);

    /* Dequeue from an empty queue: ENOENT (both out_data variants). */
    TEST_ERRNO(queue_dequeue(q, &out), ENOENT);
    TEST_ERRNO(queue_dequeue(q, NULL), ENOENT);

    /* Alternating single enqueue/dequeue: size flips 0 <-> 1 repeatedly. */
    for (int i = 0; i < 100; ++i) {
        ASSERT(queue_enqueue(q, &value) == QUEUE_OK);
        ASSERT(queue_size(q, &n) == QUEUE_OK);
        ASSERT(n == 1);
        ASSERT(queue_is_empty(q, &empty) == QUEUE_OK);
        ASSERT(empty == 0);
        int *got = NULL;
        ASSERT(queue_dequeue(q, (void **) &got) == QUEUE_OK);
        ASSERT(got == &value);
        ASSERT(queue_size(q, &n) == QUEUE_OK);
        ASSERT(n == 0);
        ASSERT(queue_is_empty(q, &empty) == QUEUE_OK);
        ASSERT(empty == 1);
        TEST_ERRNO(queue_dequeue(q, &out), ENOENT);
    }

    ASSERT(queue_destroy(q) == QUEUE_OK);
    return PASS;
}


// Destructor invocation semantics.
static int ownership(void) {
    dtor_calls = 0;
    queue_t *q = queue_create(counting_free);
    ASSERT(q != NULL);

    for (int i = 0; i < 6; ++i)
        ASSERT(queue_enqueue(q, make_int(i)) == QUEUE_OK);

    /* out_data handed out: destructor must NOT run. */
    int *out = NULL;
    ASSERT(queue_dequeue(q, (void **) &out) == QUEUE_OK);
    ASSERT(out != NULL);
    ASSERT(*out == 0);
    ASSERT(dtor_calls == 0);
    free(out);

    /* dequeue(NULL): destructor runs exactly once per element. */
    ASSERT(queue_dequeue(q, NULL) == QUEUE_OK);
    ASSERT(dtor_calls == 1);
    ASSERT(queue_dequeue(q, NULL) == QUEUE_OK);
    ASSERT(dtor_calls == 2);

    /* destroy with remaining elements: destructor once for each of the
     * three elements still queued. */
    ASSERT(queue_destroy(q) == QUEUE_OK);
    ASSERT(dtor_calls == 5);

    /* NULL destructor: dequeue(NULL) simply drops the element (no crash,
     * nothing is freed) - use statically allocated ints, not heap. */
    static int statics[3] = {10, 20, 30};
    q                     = queue_create(NULL);
    ASSERT(q != NULL);
    for (size_t i = 0; i < SIZE(statics); ++i)
        ASSERT(queue_enqueue(q, &statics[i]) == QUEUE_OK);
    ASSERT(queue_dequeue(q, NULL) == QUEUE_OK); /* statics[0] dropped */
    out = NULL;
    ASSERT(queue_dequeue(q, (void **) &out) == QUEUE_OK);
    ASSERT(out == &statics[1]);
    ASSERT(*out == 20);
    /* destroy with a static element remaining and no destructor: no crash. */
    ASSERT(queue_destroy(q) == QUEUE_OK);
    ASSERT(statics[0] == 10);
    ASSERT(statics[1] == 20);
    ASSERT(statics[2] == 30);
    return PASS;
}


// Interleaved enqueue/dequeue waves exercising the node-recycling pool.
static int stress(void) {
    enum {
        WAVES        = 8,
        ENQ_PER_WAVE = 3000,
        DEQ_PER_WAVE = 1500
    };

    /* 24000 elements enqueued in total; peak occupancy 12000. Values are
     * encoded directly in the pointer (offset by 1 to avoid NULL). */
    queue_t *q = queue_create(NULL);
    ASSERT(q != NULL);

    size_t next_in = 0, next_out = 0, n = 0;

    for (int w = 0; w < WAVES; ++w) {
        for (int i = 0; i < ENQ_PER_WAVE; ++i) {
            ASSERT(queue_enqueue(q, (void *) (uintptr_t) (next_in + 1)) ==
                   QUEUE_OK);
            ++next_in;
        }
        for (int i = 0; i < DEQ_PER_WAVE; ++i) {
            void *out = NULL;
            ASSERT(queue_dequeue(q, &out) == QUEUE_OK);
            ASSERT((uintptr_t) out == next_out + 1);
            ++next_out;
        }
        ASSERT(queue_size(q, &n) == QUEUE_OK);
        ASSERT(n == next_in - next_out);
    }

    /* Drain completely, verifying full FIFO integrity. */
    while (next_out < next_in) {
        void *out = NULL;
        ASSERT(queue_dequeue(q, &out) == QUEUE_OK);
        ASSERT((uintptr_t) out == next_out + 1);
        ++next_out;
    }
    int empty = 0;
    ASSERT(queue_is_empty(q, &empty) == QUEUE_OK);
    ASSERT(empty == 1);

    /* After the full drain the recycling pool holds thousands of nodes;
     * a further wave must reuse them and still preserve order. */
    for (int i = 0; i < 500; ++i)
        ASSERT(queue_enqueue(q, (void *) (uintptr_t) (i + 1000)) == QUEUE_OK);
    ASSERT(queue_size(q, &n) == QUEUE_OK);
    ASSERT(n == 500);
    for (int i = 0; i < 500; ++i) {
        void *out = NULL;
        ASSERT(queue_dequeue(q, &out) == QUEUE_OK);
        ASSERT((uintptr_t) out == (uintptr_t) (i + 1000));
    }

    ASSERT(queue_destroy(q) == QUEUE_OK);
    return PASS;
}


// Allocation-failure scenario: every step reports its outcome in the
// visited-points bitmap; every path frees everything it allocated.
static unsigned long scenario(void) {
    unsigned long visited = 0;
    queue_t      *q;
    int          *p[4] = {NULL, NULL, NULL, NULL};
    void         *out;
    size_t        n;
    int           empty;

    /* 0: create the queue with `free` as destructor. */
    errno = 0;
    if ((q = queue_create(free)) != NULL) visited |= V(1, 0);
    else if (errno == ENOMEM && (q = queue_create(free)) != NULL)
        visited |= V(2, 0);
    else return visited | V(4, 0);

    /* 1..6: three heap elements; each needs a malloc (odd step) and an
     * enqueue (even step), both with the fail-and-retry pattern. */
    for (size_t i = 0; i < 3; ++i) {
        unsigned mw = (unsigned) (1 + 2 * i); /* where: 1, 3, 5 */
        unsigned ew = mw + 1;                 /* where: 2, 4, 6 */

        errno = 0;
        if ((p[i] = (int *) malloc(sizeof *p[i])) != NULL) visited |= V(1, mw);
        else if (errno == ENOMEM &&
                 (p[i] = (int *) malloc(sizeof *p[i])) != NULL)
            visited |= V(2, mw);
        else {
            queue_destroy(q);
            return visited | V(4, mw);
        }
        *p[i] = (int) i;

        errno = 0;
        if (queue_enqueue(q, p[i]) == QUEUE_OK) visited |= V(1, ew);
        else if (errno == ENOMEM && queue_enqueue(q, p[i]) == QUEUE_OK)
            visited |= V(2, ew);
        else {
            free(p[i]);
            queue_destroy(q);
            return visited | V(4, ew);
        }
    }

    /* 7-9: non-allocating queries must always succeed. */
    n = 0;
    if (queue_size(q, &n) != QUEUE_OK || n != 3) {
        queue_destroy(q);
        return visited | V(4, 7);
    }
    visited |= V(1, 7);

    empty = -1;
    if (queue_is_empty(q, &empty) != QUEUE_OK || empty != 0) {
        queue_destroy(q);
        return visited | V(4, 8);
    }
    visited |= V(1, 8);

    n = 0;
    if (queue_foreach(q, count_job, &n) != QUEUE_OK || n != 3) {
        queue_destroy(q);
        return visited | V(4, 9);
    }
    visited |= V(1, 9);

    /* 10: dequeue with out_data (non-allocating); the caller frees. */
    out = NULL;
    if (queue_dequeue(q, &out) != QUEUE_OK || out != p[0]) {
        queue_destroy(q);
        return visited | V(4, 10);
    }
    free(out);
    visited |= V(1, 10);

    /* 11: dequeue(NULL) destroys p[1] via the destructor. */
    if (queue_dequeue(q, NULL) != QUEUE_OK) {
        queue_destroy(q);
        return visited | V(4, 11);
    }
    visited |= V(1, 11);

    /* 12-13: one more element; the enqueue reuses a recycled node. */
    errno = 0;
    if ((p[3] = (int *) malloc(sizeof *p[3])) != NULL) visited |= V(1, 12);
    else if (errno == ENOMEM && (p[3] = (int *) malloc(sizeof *p[3])) != NULL)
        visited |= V(2, 12);
    else {
        queue_destroy(q);
        return visited | V(4, 12);
    }
    *p[3] = 3;

    errno = 0;
    if (queue_enqueue(q, p[3]) == QUEUE_OK) visited |= V(1, 13);
    else if (errno == ENOMEM && queue_enqueue(q, p[3]) == QUEUE_OK)
        visited |= V(2, 13);
    else {
        free(p[3]);
        queue_destroy(q);
        return visited | V(4, 13);
    }

    /* 14: destroy frees the remaining p[2] and p[3] plus all nodes. */
    if (queue_destroy(q) != QUEUE_OK) return visited | V(4, 14);
    visited |= V(1, 14);

    return visited;
}


// Sweeps an injected allocation failure over every allocation site.
static int memory(void) {
    memory_tests_check();
    return memory_test(scenario);
}


/* --- Test registry --- */

static const test_list_t test_list[] = {TEST(api), TEST(edge), TEST(ownership),
                                        TEST(stress), TEST(memory)};

int main(int argc, char *argv[]) {
    return run_test_main(argc, argv, test_list, SIZE(test_list));
}
