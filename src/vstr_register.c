#define VSTR_REGISTER_C
/*
 *  Copyright (C) 1999, 2000, 2001  James Antill
 *  
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *   
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *   
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 *  email: james@and.org
 */
/* functions for registering to have call backs/cache data from the main vstr
 * code */
#include "main.h"


typedef struct Vstr_register
{
 const char *name;
 int (*add_func)(struct Vstr_base *, size_t, size_t, const void *,
                 unsigned int);
} Vstr_register;

Vstr_register *vstr__reg_ptr = NULL;
size_t vstr__reg_len = 0;


static int vstr__register_generic_add_func(struct Vstr_base *base, size_t pos,
                                           size_t len,
                                           const void *data,
                                           unsigned int type,
                                           unsigned int flags)
{

}

int vstr_register_add(const Vstr_register *reg)
{
  
}

int vstr_register_get(const char *name)
{
}

    
