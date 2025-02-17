
/* Compiler implementation of the D programming language
 * Copyright (C) 1999-2021 by The D Language Foundation, All Rights Reserved
 * written by Walter Bright
 * http://www.digitalmars.com
 * Distributed under the Boost Software License, Version 1.0.
 * http://www.boost.org/LICENSE_1_0.txt
 * https://github.com/D-Programming-Language/dmd/blob/master/src/scanelf.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "melf.hpp"

#include "mars.hpp"

/*****************************************************************************/

static char elf[4] = { 0x7F, 'E', 'L', 'F' };   // ELF file signature

/*****************************************
 * Reads an object module from base[0..buflen] and passes the names
 * of any exported symbols to (*pAddSymbol)().
 * Input:
 *      pctx            context pointer, pass to *pAddSymbol
 *      pAddSymbol      function to pass the names to
 *      base[0..buflen] contains contents of object module
 *      module_name     name of the object module (used for error messages)
 *      loc             location to use for error printing
 */

void scanElfObjModule(void* pctx, void (*pAddSymbol)(void* pctx, char* name, int pickAny), void *base, size_t buflen, const char *module_name, Loc loc)
{
    unsigned char *buf = (unsigned char *)base;
    int reason = 0;

    if (buflen < sizeof(Elf32_Ehdr))
    {
        reason = __LINE__;
      Lcorrupt:
        error(loc, "corrupt ELF object module %s %d", module_name, reason);
        return;
    }

    if (memcmp(buf, elf, 4))
    {   reason = __LINE__;
        goto Lcorrupt;
    }
    if (buf[EI_VERSION] != EV_CURRENT)
    {
        error(loc, "ELF object module %s has EI_VERSION = %d, should be %d", module_name, buf[EI_VERSION], EV_CURRENT);
        return;
    }
    if (buf[EI_DATA] != ELFDATA2LSB)
    {
        error(loc, "ELF object module %s is byte swapped and unsupported", module_name);
        return;
    }
    if (buf[EI_CLASS] == ELFCLASS32)
    {
        Elf32_Ehdr *eh = (Elf32_Ehdr *)buf;
        if (eh->e_type != ET_REL)
        {
            error(loc, "ELF object module %s is not relocatable", module_name);
            return;                             // not relocatable object module
        }
        if (eh->e_version != EV_CURRENT)
            goto Lcorrupt;

        /* For each Section
         */
        for (unsigned u = 0; u < eh->e_shnum; u++)
        {   Elf32_Shdr *section = (Elf32_Shdr *)(buf + eh->e_shoff + eh->e_shentsize * u);

            if (section->sh_type == SHT_SYMTAB)
            {   /* sh_link gives the particular string table section
                 * used for the symbol names.
                 */
                Elf32_Shdr *string_section = (Elf32_Shdr *)(buf + eh->e_shoff +
                    eh->e_shentsize * section->sh_link);
                if (string_section->sh_type != SHT_STRTAB)
                {
                    reason = __LINE__;
                    goto Lcorrupt;
                }
                char *string_tab = (char *)(buf + string_section->sh_offset);

                for (unsigned offset = 0; offset < section->sh_size; offset += sizeof(Elf32_Sym))
                {   Elf32_Sym *sym = (Elf32_Sym *)(buf + section->sh_offset + offset);

                    if (((sym->st_info >> 4) == STB_GLOBAL ||
                         (sym->st_info >> 4) == STB_WEAK) &&
                        sym->st_shndx != SHN_UNDEF)     // not extern
                    {
                        char *name = string_tab + sym->st_name;
                        //printf("sym st_name = x%x\n", sym->st_name);
                        (*pAddSymbol)(pctx, name, 1);
                    }
                }
            }
        }
    }
    else if (buf[EI_CLASS] == ELFCLASS64)
    {
        Elf64_Ehdr *eh = (Elf64_Ehdr *)buf;
        if (buflen < sizeof(Elf64_Ehdr))
            goto Lcorrupt;
        if (eh->e_type != ET_REL)
        {
            error(loc, "ELF object module %s is not relocatable", module_name);
            return;                             // not relocatable object module
        }
        if (eh->e_version != EV_CURRENT)
        {   reason = __LINE__;
            goto Lcorrupt;
        }

        /* For each Section
         */
        for (unsigned u = 0; u < eh->e_shnum; u++)
        {   Elf64_Shdr *section = (Elf64_Shdr *)(buf + eh->e_shoff + eh->e_shentsize * u);

            if (section->sh_type == SHT_SYMTAB)
            {   /* sh_link gives the particular string table section
                 * used for the symbol names.
                 */
                Elf64_Shdr *string_section = (Elf64_Shdr *)(buf + eh->e_shoff +
                    eh->e_shentsize * section->sh_link);
                if (string_section->sh_type != SHT_STRTAB)
                {
                    reason = 3;
                    goto Lcorrupt;
                }
                char *string_tab = (char *)(buf + string_section->sh_offset);

                for (unsigned offset = 0; offset < section->sh_size; offset += sizeof(Elf64_Sym))
                {   Elf64_Sym *sym = (Elf64_Sym *)(buf + section->sh_offset + offset);

                    if (((sym->st_info >> 4) == STB_GLOBAL ||
                         (sym->st_info >> 4) == STB_WEAK) &&
                        sym->st_shndx != SHN_UNDEF)     // not extern
                    {
                        char *name = string_tab + sym->st_name;
                        //printf("sym st_name = x%x\n", sym->st_name);
                        (*pAddSymbol)(pctx, name, 1);
                    }
                }
            }
        }
    }
    else
    {
        error(loc, "ELF object module %s is unrecognized class %d", module_name, buf[EI_CLASS]);
        return;
    }
}

