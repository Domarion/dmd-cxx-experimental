
/* Compiler implementation of the D programming language
 * Copyright (C) 1999-2021 by The D Language Foundation, All Rights Reserved
 * written by Walter Bright
 * http://www.digitalmars.com
 * Distributed under the Boost Software License, Version 1.0.
 * http://www.boost.org/LICENSE_1_0.txt
 * https://github.com/D-Programming-Language/dmd/blob/master/src/toctype.c
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "mars.hpp"
#include "module.hpp"
#include "mtype.hpp"
#include "declaration.hpp"
#include "enum.hpp"
#include "aggregate.hpp"
#include "id.hpp"

#include "cc.hpp"
#include "global.hpp"
#include "type.hpp"

void slist_add(Symbol *s);
void slist_reset();
unsigned totym(Type *tx);

/***************************************
 * Convert from D type to C type.
 * This is done so C debug info can be generated.
 */

type *Type_toCtype(Type *t);

class ToCtypeVisitor : public Visitor
{
public:
    ToCtypeVisitor() {}

    void visit(Type *t)
    {
        t->ctype = type_fake(totym(t));
        t->ctype->Tcount++;
    }

    void visit(TypeSArray *t)
    {
        t->ctype = type_static_array(t->dim->toInteger(), Type_toCtype(t->next));
    }

    void visit(TypeDArray *t)
    {
        t->ctype = type_dyn_array(Type_toCtype(t->next));
        t->ctype->Tident = t->toPrettyChars(true);
    }

    void visit(TypeAArray *t)
    {
        t->ctype = type_assoc_array(Type_toCtype(t->index), Type_toCtype(t->next));
    }

    void visit(TypePointer *t)
    {
        //printf("TypePointer::toCtype() %s\n", t->toChars());
        t->ctype = type_pointer(Type_toCtype(t->next));
    }

    void visit(TypeFunction *t)
    {
        size_t nparams = t->parameterList.length();

        type *tmp[10];
        type **ptypes = tmp;
        if (nparams > 10)
            ptypes = (type **)malloc(sizeof(type*) * nparams);

        for (size_t i = 0; i < nparams; i++)
        {
            Parameter *p = t->parameterList[i];
            type *tp = Type_toCtype(p->type);
            if (p->storageClass & (STCout | STCref))
                tp = type_allocn(TYnref, tp);
            else if (p->storageClass & STClazy)
            {
                // Mangle as delegate
                type *tf = type_function(TYnfunc, nullptr, 0, false, tp);
                tp = type_delegate(tf);
            }
            ptypes[i] = tp;
        }

        t->ctype = type_function(totym(t), ptypes, nparams, t->parameterList.varargs == 1, Type_toCtype(t->next));

        if (nparams > 10)
            free(ptypes);
    }

    void visit(TypeDelegate *t)
    {
        t->ctype = type_delegate(Type_toCtype(t->next));
    }

    void visit(TypeStruct *t)
    {
        //printf("TypeStruct::toCtype() '%s'\n", t->sym->toChars());
        Type *tm = t->mutableOf();
        if (tm->ctype)
        {
            t->ctype = type_alloc(tybasic(tm->ctype->Tty));
            t->ctype->Tcount++;
            if (t->ctype->Tty == TYstruct)
            {
                Symbol *s = tm->ctype->Ttag;
                t->ctype->Ttag = (Classsym *)s;            // structure tag name
            }
            // Add modifiers
            switch (t->mod)
            {
                case 0:
                    assert(0);
                    break;
                case MODconst:
                case MODwild:
                case MODwildconst:
                    t->ctype->Tty |= mTYconst;
                    break;
                case MODshared:
                    t->ctype->Tty |= mTYshared;
                    break;
                case MODshared | MODconst:
                case MODshared | MODwild:
                case MODshared | MODwildconst:
                    t->ctype->Tty |= mTYshared | mTYconst;
                    break;
                case MODimmutable:
                    t->ctype->Tty |= mTYimmutable;
                    break;
                default:
                    assert(0);
            }
        }
        else
        {
            StructDeclaration *sym = t->sym;
            t->ctype = type_struct_class(sym->toPrettyChars(true), sym->alignsize, sym->structsize,
                    sym->arg1type ? Type_toCtype(sym->arg1type) : nullptr,
                    sym->arg2type ? Type_toCtype(sym->arg2type) : nullptr,
                    sym->isUnionDeclaration() != 0,
                    false,
                    sym->isPOD() != 0);

            tm->ctype = t->ctype;

            /* Add in fields of the struct
             * (after setting ctype to avoid infinite recursion)
             */
            if (global.params.symdebug)
            {
                for (size_t i = 0; i < sym->fields.length; i++)
                {
                    VarDeclaration *v = sym->fields[i];
                    symbol_struct_addField(t->ctype->Ttag, v->ident->toChars(), Type_toCtype(v->type), v->offset);
                }
            }
        }

        //printf("t = %p, Tflags = x%x\n", ctype, ctype->Tflags);
    }

    void visit(TypeEnum *t)
    {
        //printf("TypeEnum::toCtype() '%s'\n", t->sym->toChars());
        Type *tm = t->mutableOf();
        if (tm->ctype && tybasic(tm->ctype->Tty) == TYenum)
        {
            Symbol *s = tm->ctype->Ttag;
            assert(s);
            t->ctype = type_alloc(TYenum);
            t->ctype->Ttag = (Classsym *)s;            // enum tag name
            t->ctype->Tcount++;
            t->ctype->Tnext = tm->ctype->Tnext;
            t->ctype->Tnext->Tcount++;
            // Add modifiers
            switch (t->mod)
            {
                case 0:
                    assert(0);
                    break;
                case MODconst:
                case MODwild:
                case MODwildconst:
                    t->ctype->Tty |= mTYconst;
                    break;
                case MODshared:
                    t->ctype->Tty |= mTYshared;
                    break;
                case MODshared | MODconst:
                case MODshared | MODwild:
                case MODshared | MODwildconst:
                    t->ctype->Tty |= mTYshared | mTYconst;
                    break;
                case MODimmutable:
                    t->ctype->Tty |= mTYimmutable;
                    break;
                default:
                    assert(0);
            }
        }
        else if (t->sym->ident == Id::__c_long ||
                 t->sym->ident == Id::__c_complex_float ||
                 t->sym->ident == Id::__c_complex_double ||
                 t->sym->ident == Id::__c_complex_real)
        {
            t->ctype = type_fake(totym(t));
            t->ctype->Tcount++;
        }
        else if (t->sym->memtype->toBasetype()->ty == Tint32)
        {
            t->ctype = type_enum(t->sym->toPrettyChars(true), Type_toCtype(t->sym->memtype));
            tm->ctype = t->ctype;
        }
        else
        {
            t->ctype = Type_toCtype(t->sym->memtype);
        }

        //printf("t = %p, Tflags = x%x\n", t, t->Tflags);
    }

    void visit(TypeClass *t)
    {
        //printf("TypeClass::toCtype() %s\n", toChars());
        type *tc = type_struct_class(t->sym->toPrettyChars(true), t->sym->alignsize, t->sym->structsize,
                nullptr,
                nullptr,
                false,
                true,
                true);

        t->ctype = type_pointer(tc);

        /* Add in fields of the class
         * (after setting ctype to avoid infinite recursion)
         */
        if (global.params.symdebug)
        {
            for (size_t i = 0; i < t->sym->fields.length; i++)
            {
                VarDeclaration *v = t->sym->fields[i];
                symbol_struct_addField(tc->Ttag, v->ident->toChars(), Type_toCtype(v->type), v->offset);
            }
        }
    }
};

type *Type_toCtype(Type *t)
{
    if (!t->ctype)
    {
        ToCtypeVisitor v;
        t->accept(&v);
    }
    return t->ctype;
}
