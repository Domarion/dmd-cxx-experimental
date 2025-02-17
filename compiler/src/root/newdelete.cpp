
/* Copyright (C) 2000-2021 by The D Language Foundation, All Rights Reserved
 * written by Walter Bright
 * http://www.digitalmars.com
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
 * https://github.com/dlang/dmd/blob/master/src/root/rmem.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__has_feature)
# if __has_feature(address_sanitizer)
# define USE_ASAN_NEW_DELETE
# endif
#elif defined(__SANITIZE_ADDRESS__)
# define USE_ASAN_NEW_DELETE
#endif

#if !defined(USE_ASAN_NEW_DELETE)

extern "C"
{
    void *allocmemory(size_t m_size);
}

void * operator new(size_t m_size)
{
    return allocmemory(m_size);
}

void operator delete(void *p)
{
}

#endif
