// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
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

	if (!path_file)
		fail("open gpio-sim path");
	if (!fgets(path, sizeof(path), path_file))
		fail("read gpio-sim path");
	fclose(path_file);
	path[strcspn(path, "\r\n")] = '\0';
	value_file = fopen(path, "w");
	if (!value_file)
		fail("open gpio-sim pull");
	if (fprintf(value_file, "%s\n", value) < 0 || fclose(value_file))
		fail("write gpio-sim pull");
}

static pid_t trigger_after_delay(const char *pull)
{
	pid_t child = fork();

	if (child < 0)
		fail("fork");
	if (child == 0) {
		struct timespec delay = { .tv_nsec = 20 * 1000 * 1000 };

		nanosleep(&delay, NULL);
		write_pull(pull);
		_exit(EXIT_SUCCESS);
	}
	return child;
}

static struct irqrec_event_v1 read_event(int fd, pid_t child)
{
	struct irqrec_event_v1 event;
	int child_status = 0;
	int child_reaped = 0;
	ssize_t bytes;
	int elapsed_ms = 0;

	while (elapsed_ms < 5000) {
		pid_t result;
		struct timespec retry_delay = { .tv_nsec = 50 * 1000 * 1000 };

		if (!child_reaped) {
			result = waitpid(child, &child_status, WNOHANG);
			if (result < 0)
				fail("waitpid");
			if (result == child) {
				child_reaped = 1;
				if (!WIFEXITED(child_status) || WEXITSTATUS(child_status) != 0) {
					errno = EIO;
					fail("edge stimulus");
				}
			}
		}
		bytes = read(fd, &event, sizeof(event));
		if (bytes == (ssize_t)sizeof(event))
			break;
		if (bytes >= 0 || (errno != EAGAIN && errno != EINTR))
			fail("read event");
		nanosleep(&retry_delay, NULL);
		elapsed_ms += 50;
	}
	if (bytes != (ssize_t)sizeof(event)) {
		errno = ETIMEDOUT;
		fail("read event");
	}
	if (!child_reaped && waitpid(child, &child_status, 0) < 0)
		fail("waitpid");
	if (!child_reaped && (!WIFEXITED(child_status) || WEXITSTATUS(child_status) != 0)) {
		errno = EIO;
		fail("edge stimulus");
	}

	return event;
}

static void assert_no_event(int fd)
{
	pid_t child = fork();
	int status;

	if (child < 0)
		fail("fork no-event check");
	if (child == 0) {
		struct irqrec_event_v1 event;
		ssize_t bytes;

		alarm(1);
		bytes = read(fd, &event, sizeof(event));
		if (bytes < 0 && errno == EAGAIN)
			_exit(EXIT_SUCCESS);
		if (bytes == (ssize_t)sizeof(event))
			_exit(2);
		_exit(3);
	}
	if (waitpid(child, &status, 0) < 0)
		fail("waitpid no-event check");
	if ((WIFEXITED(status) && WEXITSTATUS(status) == 0) ||
	    (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM))
		return;
	fprintf(stderr, "debounced edge was incorrectly delivered\n");
	exit(EXIT_FAILURE);
}

int main(void)
{
	struct irqrec_info_v1 info;
	struct irqrec_stats_v1 before_debounce;
	struct irqrec_stats_v1 stats;
	struct irqrec_event_v1 rising;
	struct irqrec_event_v1 falling;
	int fd = open(DEVICE, O_RDONLY | O_CLOEXEC | O_NONBLOCK);

	_Static_assert(sizeof(struct irqrec_event_v1) == 32, "event ABI changed");
	if (fd < 0)
		fail("open irqrec0");
	if (ioctl(fd, IRQREC_IOC_GET_INFO, &info))
		fail("GET_INFO");

	rising = read_event(fd, trigger_after_delay("pull-up"));
	usleep(info.debounce_us + 1000);
	falling = read_event(fd, trigger_after_delay("pull-down"));

	if (rising.value != 1 || rising.edge != IRQREC_EDGE_RISING ||
	    falling.value != 0 || falling.edge != IRQREC_EDGE_FALLING ||
	    falling.sequence <= rising.sequence ||
	    falling.timestamp_ns <= rising.timestamp_ns) {
		fprintf(stderr, "edge identity, ordering, or timestamp check failed\n");
		return EXIT_FAILURE;
	}

	usleep(info.debounce_us + 1000);
	(void)read_event(fd, trigger_after_delay("pull-up"));
	if (ioctl(fd, IRQREC_IOC_GET_STATS, &before_debounce))
		fail("GET_STATS before debounce");
	write_pull("pull-down");
	usleep(info.debounce_us + 1000);
	if (ioctl(fd, IRQREC_IOC_GET_STATS, &stats))
		fail("GET_STATS");
	if (stats.produced < 3 ||
	    ((info.features & IRQREC_FEATURE_SW_DEBOUNCE) &&
	     (stats.debounced <= before_debounce.debounced ||
	      stats.produced != before_debounce.produced))) {
		fprintf(stderr, "event or debounce counters did not advance\n");
		return EXIT_FAILURE;
	}
	if (info.features & IRQREC_FEATURE_SW_DEBOUNCE)
		assert_no_event(fd);

	close(fd);
	while (waitpid(-1, NULL, 0) > 0) { }
	printf("irq stage passed: rising=%" PRIu64 " falling=%" PRIu64
	       " produced=%" PRIu64 " debounced=%" PRIu64 "\n",
	       (uint64_t)rising.sequence, (uint64_t)falling.sequence,
	       (uint64_t)stats.produced, (uint64_t)stats.debounced);
	return EXIT_SUCCESS;
}
