/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _BOARD_H_
#define _BOARD_H_

/* board memory regions */
#define MEMORY_NUM 5
typedef struct ST_MEMORY_TBL
{
    char             name[16];       /*name of this memory dump, uniquely*/
    unsigned int     base_addr;      /*physical address of this memory   */
    unsigned int     size;           /*real data size of this memory     */
    unsigned int     necessary;      /* 1 optional, 0 necessary          */
}ST_MEMORY_TBL_ST;

/* low-level init and partition table setup */
void board_init(void);
void board_reboot(void);
void board_getvar(const char *name, char *value);

/* keypad init */
void keypad_init(void);

/* return a linux kernel commandline */
const char *board_cmdline(void);
unsigned board_machtype(void);

/* lcd panel initialization */
struct mddi_client_caps;

void panel_poweron(void);
void panel_init(struct mddi_client_caps *caps);
void panel_backlight(int on);
extern void num_to_hex8(unsigned n, char *out);
extern unsigned hex2unsigned(char *x);

#endif
