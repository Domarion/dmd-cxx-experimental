// Copyright (C) 1993-1998 by Symantec
// Copyright (C) 2000-2021 by The D Language Foundation, All Rights Reserved
// http://www.digitalmars.com
// Written by Walter Bright
/*
 * This source file is made available for personal use
 * only. The license is in backendlicense.txt
 * For any other uses, please contact Digital Mars.
 */

#pragma once

struct Aobject
{
    symbol *AOsym;              // symbol for active object
    targ_size_t AOoffset;       // offset from that object
    symbol *AOfunc;             // cleanup function
};


/* except.c */
void  except_init(void);
void  except_term(void);
elem *except_obj_ctor(elem *e,symbol *s,targ_size_t offset,symbol *sdtor);
elem *except_obj_dtor(elem *e,symbol *s,targ_size_t offset);
elem *except_throw_expression(void);
type *except_declaration(symbol *cv);
void  except_exception_spec(type *t);
void  except_index_set(int index);
int   except_index_get(void);
void  except_pair_setoffset(void *p,targ_size_t offset);
void  except_pair_append(void *p, int index);
void  except_push(void *p,elem *e,block *b);
void  except_pop(void *p,elem *e,block *b);
void  except_mark();
void  except_release();
symbol *except_gensym();
symbol *except_gentables();
void except_fillInEHTable(symbol *s);
void  except_reset();
