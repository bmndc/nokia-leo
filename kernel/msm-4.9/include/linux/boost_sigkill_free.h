/*include/linux/boost_sigkill_free.h
 *
 * Boost memory free for SIGKILLed process
 *
 *  Copyright (C) 2016 Huawei Technologies Co., Ltd.
 */
#ifndef _BOOST_SIGKILL_FREE_H
#define _BOOST_SIGKILL_FREE_H

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/signal.h>

#define MMF_FAST_FREEING 21

#define sig_kernel_kill(sig) ((sig) == SIGKILL)

extern void fast_free_user_mem(void);

#endif
