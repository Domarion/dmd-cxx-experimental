/*_ mem.h */
/* Copyright (C) 1986-2021 by The D Language Foundation, All Rights Reserved */
/* All Rights Reserved                                                       */
/* Written by Walter Bright                                                  */

#pragma once

#include <stdio.h> // for size_t

/*
 * Memory management routines.
 *
 * Compiling:
 * *
 *      #define MEM_NONE 1 to compile out mem, i.e. have it all drop
 *      directly to calls to malloc, free, etc.
 *
 *      #define MEM_NOMEMCOUNT 1 to remove checks on the number of free's
 *      matching the number of alloc's.
 *
 * Features always enabled:
 *
 *      o mem_init() is called at startup, and mem_term() at
 *        close, which checks to see that the number of alloc's is
 *        the same as the number of free's.
 *      o Behavior on out-of-memory conditions can be controlled
 *        via mem_setexception().
 *
 */

/********************* GLOBAL VARIABLES *************************/

extern int mem_inited;          /* != 0 if mem package is initialized.  */
                                /* Test this if you have other packages */
                                /* that depend on mem being initialized */

/********************* PUBLIC FUNCTIONS *************************/

/***********************************
 * Set behavior when mem runs out of memory.
 * Input:
 *      flag =  MEM_ABORTMSG:   Abort the program with the message
 *                              'Fatal error: out of memory' sent
 *                              to stdout. This is the default behavior.
 *              MEM_ABORT:      Abort the program with no message.
 *              MEM_RETNULL:    Return nullptr back to caller.
 *              MEM_CALLFP:     Call application-specified function.
 *                              fp must be supplied.
 *      fp                      Optional function pointer. Supplied if
 *                              (flag == MEM_CALLFP). This function returns
 *                              MEM_XXXXX, indicating what mem should do next.
 *                              The function could do things like swap
 *                              data out to disk to free up more memory.
 *      fp could also return:
 *              MEM_RETRY:      Try again to allocate the space. Be
 *                              careful not to go into an infinite loop.
 *      The type of fp is:
 *              int (*handler)(void)
 */

#if !MEM_NONE
#if __GNUC__
enum MEM_E { MEM_ABORTMSG, MEM_ABORT, MEM_RETNULL, MEM_CALLFP, MEM_RETRY };
void mem_setexception(enum MEM_E,...);
#else
#define MEM_ABORTMSG    0
#define MEM_ABORT       1
#define MEM_RETNULL     2
#define MEM_CALLFP      3
#define MEM_RETRY       4
void mem_setexception(int,...);
#endif
#endif

/****************************
 * Allocate space for string, copy string into it, and
 * return pointer to the new string.
 * This routine doesn't really belong here, but it is used so often
 * that I gave up and put it here.
 * Use:
 *      char *mem_strdup(const char *s);
 * Returns:
 *      pointer to copied string if succussful.
 *      else returns nullptr (if MEM_RETNULL)
 */

char *mem_strdup(const char *);

/**************************
 * Function so we can have a pointer to function mem_free().
 * This is needed since mem_free is sometimes defined as a macro,
 * and then the preprocessor screws up.
 * The pointer to mem_free() is used frequently with the list package.
 * Use:
 *      void mem_freefp(void *p);
 */

/***************************
 * Check for errors. This routine does a consistency check on the
 * storage allocator, looking for corrupted data. It should be called
 * when the application has CPU cycles to burn.
 * Use:
 *      void mem_check(void);
 */

void mem_check(void);

/***************************
 * Check ptr to see if it is in the range of allocated data.
 * Cause assertion failure if it isn't.
 */

void mem_checkptr(void *ptr);

/***************************
 * Allocate and return a pointer to numbytes of storage.
 * Use:
 *      void *mem_malloc(size_t numbytes);
 *      void *mem_calloc(size_t numbytes); allocated memory is cleared
 * Input:
 *      numbytes        Number of bytes to allocate
 * Returns:
 *      if (numbytes > 0)
 *              pointer to allocated data, nullptr if out of memory
 *      else
 *              return nullptr
 */

void *mem_malloc(size_t);
void *mem_calloc(size_t);

/*****************************
 * Reallocate memory.
 * Use:
 *      void *mem_realloc(void *ptr,size_t numbytes);
 */

void *mem_realloc(void *,size_t);

/*****************************
 * Free memory allocated by mem_malloc(), mem_calloc() or mem_realloc().
 * Use:
 *      void mem_free(void *ptr);
 */

void mem_free(void *);

/***************************
 * Initialize memory handler.
 * Use:
 *      void mem_init(void);
 * Output:
 *      mem_inited = 1
 */

void mem_init(void);

/***************************
 * Terminate memory handler. Useful for checking for errors.
 * Use:
 *      void mem_term(void);
 * Output:
 *      mem_inited = 0
 */

void mem_term(void);

/*******************************
 * The mem_fxxx() functions are for allocating memory that will persist
 * until program termination. The trick is that if the memory is never
 * free'd, we can do a very fast allocation. If MEM_DEBUG is on, they
 * act just like the regular mem functions, so it can be debugged.
 */

#if MEM_NONE
#define mem_fmalloc(u)  malloc(u)
#define mem_fcalloc(u)  calloc((u),1)
#define mem_ffree(p)    ((void)0)
#define mem_fstrdup(p)  strdup(p)
#else
void *mem_fmalloc(size_t);
void *mem_fcalloc(size_t);
#define mem_ffree(p)    ((void)0)
char *mem_fstrdup(const char *);

#endif

/***********************************
 * C++ stuff.
 */

/* The following stuff forms the implementation rather than the
 * definition, so ignore it.
 */

#if MEM_NONE

#define mem_inited      1
#define mem_strdup(p)   strdup(p)
#define mem_malloc(u)   malloc(u)
#define mem_calloc(u)   calloc((u),1)
#define mem_realloc(p,u)        realloc((p),(u))
#define mem_free(p)     free(p)
#define mem_freefp      free
#define mem_check()     ((void)0)
#define mem_checkptr(p) ((void)(p))
#define mem_init()      ((void)0)
#define mem_term()      ((void)0)

#include <stdlib.h>

#else

#define mem_freefp      mem_free
#define mem_check()
#define mem_checkptr(p)

#endif /* MEM_NONE  */
