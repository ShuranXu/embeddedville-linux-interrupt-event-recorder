// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "irqrec_uapi.h"

#define DEVICE "/dev/irqrec0"
#define SIM_PATH_FILE "/run/irqrec-sim-value"

static void fail(const char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

static void write_pull(const char *value)
{
	char path[512];
	FILE *path_file = fopen(SIM_PATH_FILE, "r");
	FILE *value_file;
	if (!path_file) fail("open gpio-sim path");
	if (!fgets(path, sizeof(path), path_file)) fail("read gpio-sim path");
	fclose(path_file);
	path[strcspn(path, "\r\n")] = '\0';
	value_file = fopen(path, "w");
	if (!value_file) fail("open gpio-sim pull");
	if (fprintf(value_file, "%s\n", value) < 0 || fclose(value_file))
		fail("write gpio-sim pull");
}

static void trigger_after_delay(const char *pull)
{
	pid_t child = fork();
	if (child < 0) fail("fork");
	if (child == 0) {
		struct timespec delay = { .tv_nsec = 20 * 1000 * 1000 };
		nanosleep(&delay, NULL);
		write_pull(pull);
		_exit(EXIT_SUCCESS);
	}
}

static struct irqrec_event_v1 wait_event(int fd)
{
	struct pollfd pfd = { .fd = fd, .events = POLLIN };
	struct irqrec_event_v1 event;
	int rc = poll(&pfd, 1, 2000);
	if (rc != 1 || !(pfd.revents & POLLIN)) {
		errno = ETIMEDOUT;
		fail("poll event");
	}
	if (read(fd, &event, sizeof(event)) != (ssize_t)sizeof(event))
		fail("read event");
	return event;
}

static void signal_handler(int signal_number)
{
	(void)signal_number;
}

static void check_interrupted_wait(void)
{
	pid_t child = fork();
	int status;

	if (child < 0) fail("fork interrupted-wait child");
	if (child == 0) {
		struct sigaction action = { .sa_handler = signal_handler };
		struct irqrec_event_v1 event;
		int fd;

		sigemptyset(&action.sa_mask);
		if (sigaction(SIGUSR1, &action, NULL)) _exit(10);
		fd = open(DEVICE, O_RDONLY | O_CLOEXEC);
		if (fd < 0) _exit(11);
		if (read(fd, &event, sizeof(event)) != -1 || errno != EINTR)
			_exit(12);
		close(fd);
		_exit(0);
	}
	usleep(50000);
	if (kill(child, SIGUSR1)) fail("signal blocked reader");
	if (waitpid(child, &status, 0) != child || !WIFEXITED(status) ||
	    WEXITSTATUS(status) != 0) {
		fprintf(stderr, "blocking read was not interrupted cleanly\n");
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	struct irqrec_info_v1 info;
	struct irqrec_stats_v1 stats;
	struct irqrec_event_v1 first, second, next_a, next_b, retained, companion, batch[4];
	char byte;
	int a = open(DEVICE, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	int b;

	if (a < 0) fail("open first reader");
	if (ioctl(a, IRQREC_IOC_GET_INFO, &info)) fail("GET_INFO");
	if (info.abi_version != IRQREC_ABI_VERSION || info.event_size != 32 ||
	    info.queue_depth < 2) {
		fprintf(stderr, "unexpected ABI info\n");
		return EXIT_FAILURE;
	}
	if (read(a, &byte, 1) != -1 || errno != EINVAL) {
		fprintf(stderr, "short read did not fail with EINVAL\n");
		return EXIT_FAILURE;
	}
	if (read(a, &first, sizeof(first)) != -1 || errno != EAGAIN) {
		fprintf(stderr, "empty nonblocking read did not return EAGAIN\n");
		return EXIT_FAILURE;
	}

	b = open(DEVICE, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (b < 0) fail("open second reader");
	trigger_after_delay("pull-up");
	first = wait_event(a);
	second = wait_event(b);
	if (first.sequence != second.sequence || first.timestamp_ns != second.timestamp_ns ||
	    first.value != 1 || first.edge != IRQREC_EDGE_RISING) {
		fprintf(stderr, "readers observed different event identities\n");
		return EXIT_FAILURE;
	}
	usleep(info.debounce_us + 1000);
	trigger_after_delay("pull-down");
	next_a = wait_event(a);
	next_b = wait_event(b);
	if (next_a.sequence != next_b.sequence ||
	    next_a.sequence <= first.sequence ||
	    next_a.timestamp_ns <= first.timestamp_ns ||
	    next_a.value != 0 || next_a.edge != IRQREC_EDGE_FALLING) {
		fprintf(stderr, "event ordering or monotonic timestamp failed\n");
		return EXIT_FAILURE;
	}
	usleep(info.debounce_us + 1000);
	trigger_after_delay("pull-up");
	{
		struct pollfd staged = { .fd = a, .events = POLLIN };

		if (poll(&staged, 1, 2000) != 1 || !(staged.revents & POLLIN)) {
			errno = ETIMEDOUT;
			fail("poll staged EFAULT event");
		}
	}
	errno = 0;
	if (syscall(SYS_read, a, (void *)(uintptr_t)1, sizeof(retained)) != -1 ||
	    errno != EFAULT) {
		fprintf(stderr, "invalid userspace pointer did not return EFAULT\n");
		return EXIT_FAILURE;
	}
	retained = wait_event(a);
	companion = wait_event(b);
	if (retained.sequence != companion.sequence || retained.value != 1 ||
	    retained.edge != IRQREC_EDGE_RISING) {
		fprintf(stderr, "event was not retained after EFAULT\n");
		return EXIT_FAILURE;
	}
	check_interrupted_wait();

	for (unsigned int i = 0; i < info.queue_depth + 8; ++i) {
		write_pull((i & 1) ? "pull-up" : "pull-down");
		usleep(info.debounce_us + 1000);
	}
	write_pull("pull-down");
	write_pull("pull-up");
	usleep(info.debounce_us + 1000);
	if (ioctl(a, IRQREC_IOC_GET_STATS, &stats)) fail("GET_STATS");
	if (stats.produced == 0 || stats.dropped == 0 ||
	    ((info.features & IRQREC_FEATURE_SW_DEBOUNCE) && stats.debounced == 0)) {
		fprintf(stderr, "expected counters missing: produced=%" PRIu64
			" dropped=%" PRIu64 " debounced=%" PRIu64
			" features=0x%" PRIx64 "\n",
			(uint64_t)stats.produced, (uint64_t)stats.dropped,
			(uint64_t)stats.debounced, (uint64_t)info.features);
		return EXIT_FAILURE;
	}
	ssize_t bytes = read(a, batch, sizeof(batch));
	if (bytes < (ssize_t)(2 * sizeof(batch[0])) ||
	    bytes % (ssize_t)sizeof(batch[0]) ||
	    !(batch[0].flags & IRQREC_EVENT_F_OVERFLOW)) {
		fprintf(stderr, "batched read or next-record overflow marker failed\n");
		return EXIT_FAILURE;
	}

	printf("ABI=%u event_size=%u depth=%u debounce_us=%u\n",
	       info.abi_version, info.event_size, info.queue_depth, info.debounce_us);
	printf("multi_reader_sequence=%" PRIu64 " produced=%" PRIu64
	       " delivered=%" PRIu64 " dropped=%" PRIu64
	       " debounced=%" PRIu64 " batch_bytes=%zd\n",
	       (uint64_t)first.sequence, (uint64_t)stats.produced,
	       (uint64_t)stats.delivered, (uint64_t)stats.dropped,
	       (uint64_t)stats.debounced, bytes);
	close(b);
	close(a);
	while (waitpid(-1, NULL, 0) > 0) { }
	puts("irqrec public runtime tests passed");
	return EXIT_SUCCESS;
}

