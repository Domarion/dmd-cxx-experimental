/* Compiler implementation of the D programming language
 * Copyright (C) 1999-2021 by The D Language Foundation, All Rights Reserved
 * written by Walter Bright
 * http://www.digitalmars.com
 * Distributed under the Boost Software License, Version 1.0.
 * http://www.boost.org/LICENSE_1_0.txt
 * https://github.com/D-Programming-Language/dmd/blob/master/src/access.c
 */

#include "root/dsystem.hpp"
#include "root/root.hpp"
#include "root/rmem.hpp"

#include "errors.hpp"
#include "enum.hpp"
#include "aggregate.hpp"
#include "init.hpp"
#include "attrib.hpp"
#include "scope.hpp"
#include "id.hpp"
#include "mtype.hpp"
#include "declaration.hpp"
#include "aggregate.hpp"
#include "expression.hpp"
#include "module.hpp"
#include "template.hpp"

/* Code to do access checks
 */

bool hasPackageAccess(Scope *sc, Dsymbol *s);
bool hasPackageAccess(Module *mod, Dsymbol *s);
bool hasPrivateAccess(AggregateDeclaration *ad, Dsymbol *smember);
bool isFriendOf(AggregateDeclaration *ad, AggregateDeclaration *cd);
static Dsymbol *mostVisibleOverload(Dsymbol *s);

/****************************************
 * Return Visibility access for Dsymbol smember in this declaration.
 */
Visibility getAccess(AggregateDeclaration *ad, Dsymbol *smember)
{
    Visibility access_ret = Visibility(Visibility::none);

    assert(ad->isStructDeclaration() || ad->isClassDeclaration());
    if (smember->toParent() == ad)
    {
        access_ret = smember->prot();
    }
    else if (smember->isDeclaration()->isStatic())
    {
        access_ret = smember->prot();
    }
    if (ClassDeclaration *cd = ad->isClassDeclaration())
    {
        for (size_t i = 0; i < cd->baseclasses->length; i++)
        {
            BaseClass *b = (*cd->baseclasses)[i];

            Visibility access = getAccess(b->sym, smember);
            switch (access.kind)
            {
                case Visibility::none:
                    break;

                case Visibility::private_:
                    access_ret = Visibility(Visibility::none);  // private members of base class not accessible
                    break;

                case Visibility::package_:
                case Visibility::protected_:
                case Visibility::public_:
                case Visibility::export_:
                    // If access is to be tightened
                    if (Visibility::public_ < access.kind)
                        access = Visibility(Visibility::public_);

                    // Pick path with loosest access
                    if (access_ret.isMoreRestrictiveThan(access))
                        access_ret = access;
                    break;

                default:
                    assert(0);
            }
        }
    }

    return access_ret;
}

/********************************************************
 * Helper function for checkAccess()
 * Returns:
 *      false   is not accessible
 *      true    is accessible
 */
static bool isAccessible(
        Dsymbol *smember,
        Dsymbol *sfunc,
        AggregateDeclaration *dthis,
        AggregateDeclaration *cdscope)
{
    assert(dthis);

    if (hasPrivateAccess(dthis, sfunc) ||
        isFriendOf(dthis, cdscope))
    {
        if (smember->toParent() == dthis)
            return true;

        if (ClassDeclaration *cdthis = dthis->isClassDeclaration())
        {
            for (size_t i = 0; i < cdthis->baseclasses->length; i++)
            {
                BaseClass *b = (*cdthis->baseclasses)[i];
                Visibility access = getAccess(b->sym, smember);
                if (access.kind >= Visibility::protected_ ||
                    isAccessible(smember, sfunc, b->sym, cdscope))
                {
                    return true;
                }
            }
        }
    }
    else
    {
        if (smember->toParent() != dthis)
        {
            if (ClassDeclaration *cdthis = dthis->isClassDeclaration())
            {
                for (size_t i = 0; i < cdthis->baseclasses->length; i++)
                {
                    BaseClass *b = (*cdthis->baseclasses)[i];
                    if (isAccessible(smember, sfunc, b->sym, cdscope))
                        return true;
                }
            }
        }
    }
    return false;
}

/*******************************
 * Do access check for member of this class, this class being the
 * type of the 'this' pointer used to access smember.
 * Returns true if the member is not accessible.
 */
bool checkAccess(AggregateDeclaration *ad, Loc loc, Scope *sc, Dsymbol *smember)
{
    FuncDeclaration *f = sc->func;
    AggregateDeclaration *cdscope = sc->getStructClassScope();

    Dsymbol *smemberparent = smember->toParent();
    if (!smemberparent || !smemberparent->isAggregateDeclaration())
    {
        return false;                   // then it is accessible
    }

    // BUG: should enable this check
    //assert(smember->parent->isBaseOf(this, nullptr));

    bool result;
    Visibility access;
    if (smemberparent == ad)
    {
        access = smember->prot();
        result = access.kind >= Visibility::public_ ||
                 hasPrivateAccess(ad, f) ||
                 isFriendOf(ad, cdscope) ||
                 (access.kind == Visibility::package_ && hasPackageAccess(sc, smember)) ||
                 ad->getAccessModule() == sc->_module;
    }
    else if ((access = getAccess(ad, smember)).kind >= Visibility::public_)
    {
        result = true;
    }
    else if (access.kind == Visibility::package_ && hasPackageAccess(sc, ad))
    {
        result = true;
    }
    else
    {
        result = isAccessible(smember, f, ad, cdscope);
    }
    if (!result)
    {
        ad->error(loc, "member %s is not accessible", smember->toChars());
        //printf("smember = %s %s, prot = %d, semanticRun = %d\n",
        //        smember->kind(), smember->toPrettyChars(), smember->prot(), smember->semanticRun);
        return true;
    }
    return false;
}

/****************************************
 * Determine if this is the same or friend of cd.
 */
bool isFriendOf(AggregateDeclaration *ad, AggregateDeclaration *cd)
{
    if (ad == cd)
        return true;

    // Friends if both are in the same module
    //if (toParent() == cd->toParent())
    if (cd && ad->getAccessModule() == cd->getAccessModule())
    {
        return true;
    }

    return false;
}

/****************************************
 * Determine if scope sc has package level access to s.
 */
bool hasPackageAccess(Scope *sc, Dsymbol *s)
{
    return hasPackageAccess(sc->_module, s);
}

bool hasPackageAccess(Module *mod, Dsymbol *s)
{
    Package *pkg = nullptr;

    if (s->prot().pkg)
        pkg = s->prot().pkg;
    else
    {
        // no explicit package for protection, inferring most qualified one
        for (; s; s = s->parent)
        {
            if (Module *m = s->isModule())
            {
                DsymbolTable *dst = Package::resolve(m->md ? m->md->packages : nullptr, nullptr, nullptr);
                assert(dst);
                Dsymbol *s2 = dst->lookup(m->ident);
                assert(s2);
                Package *p = s2->isPackage();
                if (p && p->isPackageMod())
                {
                    pkg = p;
                    break;
                }
            }
            else if ((pkg = s->isPackage()) != nullptr)
                break;
        }
    }

    if (pkg)
    {
        if (pkg == mod->parent)
        {
            return true;
        }
        if (pkg->isPackageMod() == mod)
        {
            return true;
        }
        Dsymbol* ancestor = mod->parent;
        for (; ancestor; ancestor = ancestor->parent)
        {
            if (ancestor == pkg)
            {
                return true;
            }
        }
    }

    return false;
}

/****************************************
 * Determine if scope sc has protected level access to cd.
 */
bool hasProtectedAccess(Scope *sc, Dsymbol *s)
{
    if (ClassDeclaration *cd = s->isClassMember()) // also includes interfaces
    {
        for (Scope *scx = sc; scx; scx = scx->enclosing)
        {
            if (!scx->scopesym)
                continue;
            ClassDeclaration *cd2 = scx->scopesym->isClassDeclaration();
            if (cd2 && cd->isBaseOf(cd2, nullptr))
                return true;
        }
    }
    return sc->_module == s->getAccessModule();
}

/**********************************
 * Determine if smember has access to private members of this declaration.
 */
bool hasPrivateAccess(AggregateDeclaration *ad, Dsymbol *smember)
{
    if (smember)
    {
        AggregateDeclaration *cd = nullptr;
        Dsymbol *smemberparent = smember->toParent();
        if (smemberparent)
            cd = smemberparent->isAggregateDeclaration();

        if (ad == cd)         // smember is a member of this class
        {
            return true;           // so we get private access
        }

        // If both are members of the same module, grant access
        while (1)
        {
            Dsymbol *sp = smember->toParent();
            if (sp->isFuncDeclaration() && smember->isFuncDeclaration())
                smember = sp;
            else
                break;
        }
        if (!cd && ad->toParent() == smember->toParent())
        {
            return true;
        }
        if (!cd && ad->getAccessModule() == smember->getAccessModule())
        {
            return true;
        }
    }
    return false;
}

/****************************************
 * Check access to d for expression e.d
 * Returns true if the declaration is not accessible.
 */
bool checkAccess(Loc loc, Scope *sc, Expression *e, Declaration *d)
{
    if (sc->flags & SCOPEnoaccesscheck)
        return false;

    if (d->isUnitTestDeclaration())
    {
        // Unittests are always accessible.
        return false;
    }
    if (!e)
        return false;

    if (e->type->ty == Tclass)
    {
        // Do access check
        ClassDeclaration *cd = (ClassDeclaration *)(((TypeClass *)e->type)->sym);
        if (e->op == TOKsuper)
        {
            ClassDeclaration *cd2 = sc->func->toParent()->isClassDeclaration();
            if (cd2)
                cd = cd2;
        }
        return checkAccess(cd, loc, sc, d);
    }
    else if (e->type->ty == Tstruct)
    {
        // Do access check
        StructDeclaration *cd = (StructDeclaration *)(((TypeStruct *)e->type)->sym);
        return checkAccess(cd, loc, sc, d);
    }
    return false;
}

/****************************************
 * Check access to package/module `p` from scope `sc`.
 *
 * Params:
 *   loc = source location for issued error message
 *   sc = scope from which to access to a fully qualified package name
 *   p = the package/module to check access for
 * Returns: true if the package is not accessible.
 *
 * Because a global symbol table tree is used for imported packages/modules,
 * access to them needs to be checked based on the imports in the scope chain
 * (see Bugzilla 313).
 *
 */
bool checkAccess(Scope *sc, Package *p)
{
    if (sc->_module == p)
        return false;
    for (; sc; sc = sc->enclosing)
    {
        if (sc->scopesym && sc->scopesym->isPackageAccessible(p, Visibility(Visibility::private_)))
            return false;
    }

    return true;
}

/**
 * Check whether symbols `s` is visible in `mod`.
 *
 * Params:
 *  mod = lookup origin
 *  s = symbol to check for visibility
 * Returns: true if s is visible in mod
 */
bool symbolIsVisible(Module *mod, Dsymbol *s)
{
    // should sort overloads by ascending protection instead of iterating here
    s = mostVisibleOverload(s);

    switch (s->prot().kind)
    {
        case Visibility::undefined:
            return true;
        case Visibility::none:
            return false; // no access
        case Visibility::private_:
            return s->getAccessModule() == mod;
        case Visibility::package_:
            return s->getAccessModule() == mod || hasPackageAccess(mod, s);
        case Visibility::protected_:
            return s->getAccessModule() == mod;
        case Visibility::public_:
        case Visibility::export_:
            return true;
        default:
            assert(0);
    }
    return false;
}

/**
 * Same as above, but determines the lookup module from symbols `origin`.
 */
bool symbolIsVisible(Dsymbol *origin, Dsymbol *s)
{
    return symbolIsVisible(origin->getAccessModule(), s);
}

/**
 * Same as above but also checks for protected symbols visible from scope `sc`.
 * Used for qualified name lookup.
 *
 * Params:
 *  sc = lookup scope
 *  s = symbol to check for visibility
 * Returns: true if s is visible by origin
 */
bool symbolIsVisible(Scope *sc, Dsymbol *s)
{
    s = mostVisibleOverload(s);

    switch (s->prot().kind)
    {
        case Visibility::undefined:
            return true;
        case Visibility::none:
            return false; // no access
        case Visibility::private_:
            return sc->_module == s->getAccessModule();
        case Visibility::package_:
            return sc->_module == s->getAccessModule() || hasPackageAccess(sc->_module, s);
        case Visibility::protected_:
            return hasProtectedAccess(sc, s);
        case Visibility::public_:
        case Visibility::export_:
            return true;
        default:
            assert(0);
    }
    return false;
}

/**
 * Use the most visible overload to check visibility. Later perform an access
 * check on the resolved overload.  This function is similar to overloadApply,
 * but doesn't recurse nor resolve aliases because protection/visibility is an
 * attribute of the alias not the aliasee.
 */
static Dsymbol *mostVisibleOverload(Dsymbol *s)
{
    if (!s->isOverloadable())
        return s;

    Dsymbol *next = nullptr;
    Dsymbol *fstart = s;
    Dsymbol *mostVisible = s;
    for (; s; s = next)
    {
        // void func() {}
        // private void func(int) {}
        if (FuncDeclaration *fd = s->isFuncDeclaration())
            next = fd->overnext;
        // template temp(T) {}
        // private template temp(T:int) {}
        else if (TemplateDeclaration *td = s->isTemplateDeclaration())
            next = td->overnext;
        // alias common = mod1.func1;
        // alias common = mod2.func2;
        else if (FuncAliasDeclaration *fa = s->isFuncAliasDeclaration())
            next = fa->overnext;
        // alias common = mod1.templ1;
        // alias common = mod2.templ2;
        else if (OverDeclaration *od = s->isOverDeclaration())
            next = od->overnext;
        // alias name = sym;
        // private void name(int) {}
        else if (AliasDeclaration *ad = s->isAliasDeclaration())
        {
            if (!ad->isOverloadable())
            {
                //printf("Non overloadable Aliasee in overload list\n");
                assert(0);
            }
            // Yet unresolved aliases store overloads in overnext.
            if (ad->semanticRun < PASSsemanticdone)
                next = ad->overnext;
            else
            {
                /* This is a bit messy due to the complicated implementation of
                 * alias.  Aliases aren't overloadable themselves, but if their
                 * Aliasee is overloadable they can be converted to an overloadable
                 * alias.
                 *
                 * This is done by replacing the Aliasee w/ FuncAliasDeclaration
                 * (for functions) or OverDeclaration (for templates) which are
                 * simply overloadable aliases w/ weird names.
                 *
                 * Usually aliases should not be resolved for visibility checking
                 * b/c public aliases to private symbols are public. But for the
                 * overloadable alias situation, the Alias (_ad_) has been moved
                 * into it's own Aliasee, leaving a shell that we peel away here.
                 */
                Dsymbol *aliasee = ad->toAlias();
                if (aliasee->isFuncAliasDeclaration() || aliasee->isOverDeclaration())
                    next = aliasee;
                else
                {
                    /* A simple alias can be at the end of a function or template overload chain.
                     * It can't have further overloads b/c it would have been
                     * converted to an overloadable alias.
                     */
                    if (ad->overnext)
                    {
                        //printf("Unresolved overload of alias\n");
                        assert(0);
                    }
                    break;
                }
            }

            // handled by overloadApply for unknown reason
            assert(next != ad); // should not alias itself
            assert(next != fstart); // should not alias the overload list itself
        }
        else
            break;

        if (next && mostVisible->prot().isMoreRestrictiveThan(next->prot()))
            mostVisible = next;
    }
    return mostVisible;
}
