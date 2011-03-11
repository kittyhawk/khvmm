/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       lock_guard.h
 * Description:     Lock Guard
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"

template <typename T>
class lock_guard_t
{
    private:
        T &l;
    public:
    
    lock_guard_t (T &lock) : l (lock)
        {
            l.lock();
        }

        ~lock_guard_t()
        {
            l.unlock();
        }
};
