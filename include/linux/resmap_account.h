/*
 * resmap_account.h
 *
 * Provide resmap_account external call interface
 *
 */

#ifndef RESMAP_ACCOUNT_H
#define RESMAP_ACCOUNT_H

#include <linux/types.h>
#include <linux/percpu.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/atomic.h>

enum resmap_item {
	RESMAP_ACTION,
	RESMAP_SUCCESS,
	RESMAP_FAIL,
	RESMAP_TEXIT,
	RESMAP_ITEMS
};

struct resmap_event_state {
	unsigned int event[RESMAP_TEXIT];
};

DECLARE_PER_CPU(struct resmap_event_state, resmap_event_states);

static inline void __count_resmap_event(enum resmap_item item)
{
	raw_cpu_inc(resmap_event_states.event[item]);
}

static inline void count_resmap_event(enum resmap_item item)
{
	this_cpu_inc(resmap_event_states.event[item]);
}
#endif
