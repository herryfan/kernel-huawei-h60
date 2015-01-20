/* include/linux/earlysuspend.h
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_EARLYSUSPEND_H
#define _LINUX_EARLYSUSPEND_H
#include <bsp_om.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/list.h>
#endif

/* The early_suspend structure defines suspend and resume hooks to be called
 * when the user visible sleep state of the system changes, and a level to
 * control the order. They can be used to turn off the screen and input
 * devices that are not used for wakeup.
 * Suspend handlers are called in low to high level order, resume handlers are
 * called in the opposite order. If, when calling register_early_suspend,
 * the suspend handlers have already been called without a matching call to the
 * resume handlers, the suspend handler will be called directly from
 * register_early_suspend. This direct call can violate the normal level order.
 */
enum {
	EARLY_SUSPEND_LEVEL_BLANK_SCREEN = 50,
	EARLY_SUSPEND_LEVEL_STOP_DRAWING = 100,
	EARLY_SUSPEND_LEVEL_DISABLE_FB = 150,
};
struct early_suspend {
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct list_head link;
	int level;
	void (*suspend)(struct early_suspend *h);
	void (*resume)(struct early_suspend *h);
#endif
};


#define earlysuspend_print_error(fmt, ...)    (bsp_trace(BSP_LOG_LEVEL_ERROR, BSP_MODU_EARLYSUSPEND, "[earlysuspend]: <%s> <%d>"fmt"\n", __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define earlysuspend_print_dbg(fmt, ...)    (bsp_trace(BSP_LOG_LEVEL_DEBUG, BSP_MODU_EARLYSUSPEND, "[earlysuspend]: <%s> <%d>"fmt"\n", __FUNCTION__, __LINE__, ##__VA_ARGS__))

#ifdef CONFIG_HAS_EARLYSUSPEND
void register_early_suspend(struct early_suspend *handler);
void unregister_early_suspend(struct early_suspend *handler);
#else
#define register_early_suspend(handler) do { } while (0)
#define unregister_early_suspend(handler) do { } while (0)
#endif

#endif

