/*********************************************************************
 *                
 * Copyright (C) 2008, Volkmar Uhlig, IBM Corporation
 *                
 * File path:     l4.h
 * Description:   L4 includes
 *                
 * All rights reserved
 *                
 ********************************************************************/
#pragma once

#include <l4/arch.h>
#include <l4/ipc.h>
#include <l4/kdebug.h>
#include <l4/kip.h>
#include <l4/sigma0.h>
#include <l4/thread.h>
#include <l4/schedule.h>

L4_INLINE L4_Word_t L4_Set_ProcessorNo_Prio(L4_ThreadId_t tid, L4_Word_t cpu_no, L4_Word_t prio)
{
    L4_Word_t dummy;
    prio &= 0xff;
    return L4_Schedule(tid, ~0UL, cpu_no, prio, ~0UL, &dummy);
}
