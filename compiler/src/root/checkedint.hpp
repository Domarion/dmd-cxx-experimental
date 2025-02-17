
/* Compiler implementation of the D programming language
 * Copyright (C) 2003-2021 by The D Language Foundation, All Rights Reserved
 * written by Walter Bright
 * http://www.digitalmars.com
 * Distributed under the Boost Software License, Version 1.0.
 * http://www.boost.org/LICENSE_1_0.txt
 * https://github.com/D-Programming-Language/dmd/blob/master/src/root/checkedint.h
 */

#include "dsystem.hpp"

uint64_t mulu(uint64_t x, uint64_t y, bool& overflow);
