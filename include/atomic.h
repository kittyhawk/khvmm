/*********************************************************************
 *                
 * Copyright (C) 2008, Volkmar Uhlig, IBM Corporation
 *                
 * File path:     atomic.h
 * Description:   Atomic operations
 *                
 * All rights reserved
 *                
 ********************************************************************/
#pragma once

#include "std.h"
#include "sync.h"

class atomic_t {
public:
    int operator ++ (int) 
	{
	    int tmp;
	    sync();
	    __asm__ __volatile__(
		"1:	lwarx	%0,0,%1\n"
		"	addic	%0,%0,1\n"
		"	stwcx.	%0,0,%1 \n"
		"	bne-	1b"
		: "=&r" (tmp)
		: "r" (&val)
		: "cc");
	    isync();
	    return tmp;
	}

    int operator -- (int) 
	{
	    int tmp;
	    sync();
	    __asm__ __volatile__(
		"1:	lwarx	%0,0,%1\n"
		"	addic	%0,%0,-1\n"
		"	stwcx.	%0,0,%1 \n"
		"	bne-	1b"
		: "=&r" (tmp)
		: "r" (&val)
		: "cc");
	    isync();
	    return tmp;
	}

    int operator = (word_t v) 
	{ return val = v; }

    int operator = (int v) 
	{ return val = v; }

    bool operator == (word_t v) 
	{ return val == v; }
    
    bool operator == (int v) 
	{ return val == (word_t) v; }

    bool operator != (word_t v) 
	{ return val != v; }

    bool operator != (int v) 
	{ return val != (word_t) v; }

    operator word_t (void) 
	{ return val; }

private:
    word_t val;
};
