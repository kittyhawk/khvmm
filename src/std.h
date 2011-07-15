/*********************************************************************
 *                
 * Copyright (C) 2008, Volkmar Uhlig, Udo Steinberg, Jan Stoess
 *                     IBM Corporation
 *                
 * File path:     std.h
 * Description:   Kittyhawk VMM implementation
 *                
 * All rights reserved
 *                
 ********************************************************************/
#ifndef __STD_H__
#define __STD_H__

#include <l4io.h>
#include "l4.h"

#define WEAK            __attribute__(( weak ))
#define PACKED          __attribute__((packed))
#define ALIGN(s)	__attribute__((aligned(s)))
#define PACKED          __attribute__((packed))

#ifndef NULL
#define NULL 0
#endif


/* Functions with this are NEVER generated standalone. 
 * They are inlined only. Carefull: if the maximum inlining 
 * limit is reached gcc 3.x does not inline even if explicitly 
 * specified. Use -finline-limit=<large number> here. */
#define INLINE extern inline

#define dprintf(id,format...)	printf(format)

#ifndef NODEBUG
#define assert(X)   do {                                                                            \
                        if (!(X)) {                                                                 \
                            printf ("Assertion \"%s\" failed at %s:%d\n", #X, __FILE__, __LINE__);  \
                            __builtin_trap();                                                       \
                        }                                                                           \
                    } while (0)                                                                      
#else
#define assert(x...)
#endif

# define unimplemented()				\
do {							\
    printf ("\nNot implemented: %s\n%s, line %d\n",	\
	    __PRETTY_FUNCTION__, __FILE__, __LINE__);	\
    for (;;)						\
	L4_KDB_Enter ("unimplemented");			\
} while (false)

/* type definitions */
typedef L4_Word64_t         u64_t;
typedef L4_Word32_t         u32_t;
typedef L4_Word16_t         u16_t;
typedef L4_Word8_t           u8_t;
typedef L4_SignedWord16_t   s16_t;
typedef L4_Word_t           word_t;
typedef L4_Size_t           size_t;
typedef L4_Word64_t         paddr_t;

typedef struct { L4_Word_t x[4] ALIGN(16); } fpu_word_t;

#define BITS_WORD	(sizeof(word_t)*8)

/* helpers */
#define isspace(c)      ((c == ' ') || (c == '\t'))
#define ULONG_MAX       (~0UL)
#define isdigit(c)      ((c) >= '0' && (c) <= '9')
#define islower(c)      (((c) >= 'a') && ((c) <= 'z'))
#define isupper(c)      (((c) >= 'A') && ((c) <= 'Z'))
#define isalpha(c)      (islower(c) || isupper(c))

#define ROUND_DOWN(x, size)	(x & ~(size-1))
#define ROUND_UP(x, size)	(ROUND_DOWN(x, size) == x ? x : ROUND_DOWN(x, size) + size)

template<typename T> inline const T& min(const T& a, const T& b)
{
    if (b < a)
        return b;
    return a;
}

template<typename T> inline const T& max(const T& a, const T& b)
{
    if (a < b)
        return b;
    return a;
}


extern "C" {
    unsigned strlen( const char *str );
    int strcmp( const char *s1, const char *s2 );
    int strncmp(const char *s1, const char *s2, int n);
    char const* strstr(const char *s, const char *find);;
    char *strncpy( char *dest, const char *src, word_t n );
    unsigned long strtoul(const char* nptr, const char** endptr, int base);
    char* strcpy( char* dest, const char* s);
    inline char *strchr (char const *s, int c)
    {
        for (char const *ptr = s; *ptr; ptr++)
            if (*ptr == c)
                return const_cast<char *>(ptr);

        return 0;
    }
    
    void *memmove( void *dest, const void *src, word_t n );
    void *memcpy (void *, const void *, size_t);
    void *memset (void *, int, size_t);
    void zero_mem( void *dest, word_t size );
    void page_zero( void *start, unsigned long size );
    extern int sprintf (char *__restrict __s,
                        __const char *__restrict __format, ...);
         //__THROW __attribute__ ((__format__ (__printf__, 2, 3)));
    //libc_hidden_proto(sprintf)

};

bool kernel_has_feature (L4_KernelInterfacePage_t *, char const *feature);
L4_ThreadId_t get_new_tid (unsigned = ~0u);

#endif /* !__STD_H__ */
