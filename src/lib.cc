/*********************************************************************
 *                
 * Copyright (C) 2008, Volkmar Uhlig, IBM Corporation
 *                
 * File path:     lib.cc
 * Description:   Kittyhawk VMM implementation
 *                
 * All rights reserved
 *                
 ********************************************************************/

#include "atomic.h"
#include "std.h"

bool kernel_has_feature (L4_KernelInterfacePage_t *kip, char const *feature)
{
    char *c;
    for (unsigned i = 0; (c = L4_Feature (kip, i)) != NULL; i++)
	    if (strstr (c, feature))
	        return true;

    return false;
}

static L4_Word_t next_tid;
static spinlock_t tid_lock;

L4_ThreadId_t get_new_tid (unsigned id)
{
    tid_lock.lock();
    if (!next_tid)           // Initialize with my thread_no + 1
        next_tid = L4_ThreadNo (L4_MyGlobalId()) + 1;
    L4_ThreadId_t tid = L4_GlobalId (next_tid++, id);
    tid_lock.unlock();

    return tid;
}

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long strtoul(const char* nptr, const char** endptr, int base)
{
	const char *s;
	unsigned long acc, cutoff;
	int c;
	int neg, any, cutlim;

	/*
	 * See strtol for comments as to the logic used.
	 */
	s = nptr;
	do {
		c = (unsigned char) *s++;
	} while (isspace(c));
	
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else {
		neg = 0;
		if (c == '+')
			c = *s++;
	}
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	cutoff = ULONG_MAX / (unsigned long)base;
	cutlim = ULONG_MAX % (unsigned long)base;
	for (acc = 0, any = 0;; c = (unsigned char) *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0)
			continue;
		if (acc > cutoff || acc == cutoff && c > cutlim) {
			any = -1;
			acc = ULONG_MAX;
		} else {
			any = 1;
			acc *= (unsigned long)base;
			acc += c;
		}
	}
	if (neg && any > 0)
		acc = -acc;
	if (endptr != 0)
		*endptr = (any ? s - 1 : nptr);
	return (acc);
}
