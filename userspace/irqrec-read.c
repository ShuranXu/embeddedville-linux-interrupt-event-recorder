// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "irqrec_uapi.h"

static const char *edge_name(unsigned int edge)
{
	switch (edge) {
	case IRQREC_EDGE_RISING: return "rising";
	case IRQREC_EDGE_FALLING: return "falling";
	default: return "unknown";
	}
}

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : "/dev/irqrec0";
	struct irqrec_info_v1 info;
	struct irqrec_stats_v1 stats;
	struct pollfd pfd;
	int fd;

	_Static_assert(sizeof(struct irqrec_event_v1) == 32, "event ABI changed");
	_Static_assert(sizeof(struct irqrec_info_v1) == 32, "info ABI changed");
	_Static_assert(sizeof(struct irqrec_stats_v1) == 32, "stats ABI changed");

	fd = open(path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
	if (fd < 0) {
		perror("open");
		return EXIT_FAILURE;
	}
	if (ioctl(fd, IRQREC_IOC_GET_INFO, &info) < 0) {
		perror("IRQREC_IOC_GET_INFO");
		close(fd);
		return EXIT_FAILURE;
	}
	fprintf(stderr, "irqrec ABI=%u event=%u depth=%u debounce=%uus features=0x%" PRIx64 "\n",
		info.abi_version, info.event_size, info.queue_depth,
		info.debounce_us, (uint64_t)info.features);

	pfd.fd = fd;
	pfd.events = POLLIN;
	for (;;) {
		struct irqrec_event_v1 events[16];
		ssize_t bytes;
		int ready = poll(&pfd, 1, 10000);
		if (ready < 0) {
			if (errno == EINTR) continue;
			perror("poll");
			break;
		}
		if (ready == 0) {
			fputs("waiting for a gpio-sim edge...\n", stderr);
			continue;
		}
		if (pfd.revents & (POLLHUP | POLLERR)) {
			fputs("device removed\n", stderr);
			break;
		}
		bytes = read(fd, events, sizeof(events));
		if (bytes < 0) {
			if (errno == EAGAIN || errno == EINTR) continue;
			perror("read");
			break;
		}
		if (bytes == 0) break;
		for (size_t i = 0; i < (size_t)bytes / sizeof(events[0]); ++i) {
			printf("{\"sequence\":%" PRIu64 ",\"timestamp_ns\":%" PRIu64
			       ",\"value\":%u,\"edge\":\"%s\",\"overflow\":%s}\n",
			       (uint64_t)events[i].sequence,
			       (uint64_t)events[i].timestamp_ns,
			       events[i].value, edge_name(events[i].edge),
			       events[i].flags & IRQREC_EVENT_F_OVERFLOW ? "true" : "false");
			fflush(stdout);
		}
	}

	if (ioctl(fd, IRQREC_IOC_GET_STATS, &stats) == 0)
		fprintf(stderr, "produced=%" PRIu64 " debounced=%" PRIu64
			" delivered=%" PRIu64 " dropped=%" PRIu64 "\n",
			(uint64_t)stats.produced, (uint64_t)stats.debounced,
			(uint64_t)stats.delivered, (uint64_t)stats.dropped);
	close(fd);
	return EXIT_SUCCESS;
}

