

#ifndef __LINUXINTERFACE_H_
#define __LINUXINTERFACE_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/******************************************************************************
  1 其他头文件包含
******************************************************************************/
#if (VOS_OS_VER == VOS_LINUX)
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/version.h>
#include <init.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
#include <linux/if.h>
#else
#include <if.h>
#endif
#include <kernel.h>
#include <skbuff.h>
#include <netdevice.h>
#include <asm-generic/errno-base.h>
#include <delay.h>
#include <gfp.h>
#include <linux/netlink.h>
#else
#include "LinuxStub.h"
#endif

/******************************************************************************
  2 宏定义
******************************************************************************/
#ifdef __UT_CENTER__
#define ACPU_FID_RNIC    (0)
#endif

/******************************************************************************
  3 枚举定义
******************************************************************************/


/******************************************************************************
  4 全局变量声明
******************************************************************************/


/******************************************************************************
  5 消息头定义
******************************************************************************/


/******************************************************************************
  6 消息定义
******************************************************************************/


/******************************************************************************
  7 STRUCT定义
******************************************************************************/
#pragma pack(4)



#if ((VOS_OS_VER == VOS_WIN32) || (VOS_OS_VER == VOS_NUCLEUS))
#pragma pack()
#else
#pragma pack(0)
#endif


/******************************************************************************
  8 UNION定义
******************************************************************************/


/******************************************************************************
  9 OTHERS定义
******************************************************************************/




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* LINUXInterface.h */






