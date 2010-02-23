/*********************************************************************
 *                
 * Copyright (C) 2005, 2008,  University of Karlsruhe
 *                
 * File path:     memory.cc
 * Description:   Basic memory block manipulations, such as copy and clear.
 *                
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *                
 ********************************************************************/

#include "std.h"

WEAK void *memset (void *dst, int c, size_t n)
{
    char *d = static_cast<char *>(dst);
    
    while (n--)
        *d++ = c;
    
    return dst;
}
 
WEAK void *memcpy (void *dst, const void *src, size_t n)
{
    char *d = static_cast<char *>(dst);
    char const *s = static_cast<char const *>(src);
    
    while (n--)
        *d++ = *s++;
    
    return dst;
}

WEAK unsigned strlen( const char *str )
{
    unsigned len = 0;
    while( str[len] )
	len++;
    return len;
}

WEAK int strncmp(const char *s1, const char *s2, int n)
{
    if (n == 0)
	return (0);
    do {
	if (*s1 != *s2++)
	    return (*(unsigned char *)s1 - *(unsigned char *)--s2);
	if (*s1++ == 0)
	    break;
    } while (--n != 0);
    return 0;
}

WEAK int strcmp( const char *str1, const char *str2 )
{
    while( *str1 && *str2 )
    {
	if( *str1 < *str2 )
	    return -1;
	if( *str1 > *str2 )
	    return 1;
	str1++;
	str2++;
    }
    if( *str2 )
	return -1;
    if( *str1 )
	return 1;
    return 0;
}

/*
 * Find the first occurrence of find in s.
 */
WEAK char const* strstr(const char *s, const char *find)
{
    char c, sc;
    int len;

    if ((c = *find++) != 0) {
	len = strlen(find);
	do {
	    do {
		if ((sc = *s++) == 0)
		    return (0);
	    } while (sc != c);
	} while (strncmp(s, find, len) != 0);
	s--;
    }

    return s;
}
