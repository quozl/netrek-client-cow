/* test_event_pipe.c — tests for the self-pipe signaling mechanism */

#include "unity/unity.h"
#include "../Wlib.h"
#include <unistd.h>
#include <fcntl.h>

static void setUp(void) {}
static void tearDown(void) {}

/* W_Initialize sets up the pipe; W_Socket() returns the read end fd */
static void test_socket_returns_valid_fd(void) {
    int fd = W_Socket();
    TEST_ASSERT_GREATER_THAN(-1, fd);
}

/* The pipe read end must be non-blocking (O_NONBLOCK) */
static void test_pipe_read_end_nonblocking(void) {
    int fd = W_Socket();
    int flags = fcntl(fd, F_GETFL, 0);
    TEST_ASSERT_TRUE(flags & O_NONBLOCK);
}

/* W_EventsQueuedCk must not crash when called with an empty SDL queue */
static void test_events_queued_ck_with_empty_queue(void) {
    int n = W_EventsQueuedCk();
    TEST_ASSERT_TRUE(n >= 0);
}

/* W_EventsPending must not crash and must return 0 or 1 */
static void test_events_pending_returns_boolean(void) {
    int p = W_EventsPending();
    TEST_ASSERT_TRUE(p == 0 || p == 1);
}

int main(void) {
    /* SDL_VIDEODRIVER=dummy must be set by test runner */
    W_Initialize(NULL);

    UNITY_BEGIN();
    RUN_TEST(test_socket_returns_valid_fd);
    RUN_TEST(test_pipe_read_end_nonblocking);
    RUN_TEST(test_events_queued_ck_with_empty_queue);
    RUN_TEST(test_events_pending_returns_boolean);
    return UNITY_END();
}
