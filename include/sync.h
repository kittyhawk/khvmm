/*********************************************************************
 *                
 * Copyright (C) 2008, Volkmar Uhlig, IBM Corporation
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *                
 * File path:     sync.h
 * Description:   
 *                
 * All rights reserved
 *                
 ********************************************************************/
#pragma once

extern inline void sync()
{
    asm volatile ("sync");
}

extern inline void isync()
{
    asm volatile ("isync");
}

extern inline void mbar()
{
    asm volatile ("mbar");
}

extern inline void msync()
{
    asm volatile ("msync");
}

extern inline void eieio()
{
    asm volatile ("eieio");
}

extern inline void smp_mem_barrier()
{
    asm volatile ("sync" : : : "memory");
}

class spinlock_t
{
    private:
        volatile word_t _lock;

    public:
        explicit spinlock_t() : _lock (0) {}

        void lock()
        {
            word_t old_val;

            // The only expense of a lwarx is that the CPU will snoop memory traffic.
            // So we should be able to use it in the spin.
            asm volatile ("1:	lwarx	%0, 0, %1   \n" // Load the lock
                          "     cmpwi	0, %0, 0    \n" // Is the lock 0?
                          "     bne-	1b          \n" // Retry the lock
                          "     stwcx.	%2, 0, %1   \n"	// Try to store the new lock
                          "     bne-	1b          \n" // Retry if we failed to store
                          "     isync\n"
                          : "=&r" (old_val)
                          : "r" (&_lock), "r" (1)
                          : "cr0", "memory");
        }

        void unlock()
        {
            // Ensure memory ordering before we unlock.
            asm volatile ("eieio" : : : "memory");
            _lock = 0;
        }

        bool is_locked() const { return _lock; }
};
