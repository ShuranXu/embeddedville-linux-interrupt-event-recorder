/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef IRQREC_UAPI_H
#define IRQREC_UAPI_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define IRQREC_ABI_VERSION 1U
#define IRQREC_IOC_MAGIC 0xE7

enum irqrec_edge_v1 {
	IRQREC_EDGE_UNKNOWN = 0,
	IRQREC_EDGE_RISING = 1,
	IRQREC_EDGE_FALLING = 2,
};

#define IRQREC_EVENT_F_OVERFLOW (1U << 0)
#define IRQREC_FEATURE_POLL (1ULL << 0)
#define IRQREC_FEATURE_PER_OPEN_FIFO (1ULL << 1)
#define IRQREC_FEATURE_THREADED_IRQ (1ULL << 2)
#define IRQREC_FEATURE_SW_DEBOUNCE (1ULL << 3)

struct irqrec_event_v1 {
	__u64 sequence;
	__u64 timestamp_ns;
	__u8 value;
	__u8 edge;
	__u16 flags;
	__u32 reserved[3];
};

struct irqrec_info_v1 {
	__u32 abi_version;
	__u32 event_size;
	__u32 queue_depth;
	__u32 debounce_us;
	__u64 features;
	__u64 reserved;
};

struct irqrec_stats_v1 {
	__u64 produced;
	__u64 debounced;
	__u64 delivered;
	__u64 dropped;
};

#define IRQREC_IOC_GET_INFO _IOR(IRQREC_IOC_MAGIC, 0x00, struct irqrec_info_v1)
#define IRQREC_IOC_GET_STATS _IOR(IRQREC_IOC_MAGIC, 0x01, struct irqrec_stats_v1)

#endif

