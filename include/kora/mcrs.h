/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#ifndef _KORA_MCRS_H
#define _KORA_MCRS_H 1

#define _Kib_ (1024L)
#define _Mib_ (1024L*_Kib_)
#define _Gib_ (1024LL*_Mib_)
#define _Tib_ (1024LL*_Gib_)
#define _Pib_ (1024LL*_Tib_)
#define _Eib_ (1024LL*_Pib_)

#define _PwMilli_ (1000L)
#define _PwMicro_ (1000L * _PwMilli_)
#define _PwNano_ (1000LL * _PwMicro_)
#define _PwFemto_ (1000LL * _PwNano_)

#define ALIGN_UP(v,a)      (((v)+(a-1))&(~(a-1)))
#define ALIGN_DW(v,a)      ((v)&(~(a-1)))
#define IS_ALIGNED(v,a)      (((v)&((a)-1))==0)

#define ADDR_OFF(a,o)  ((void*)(((char*)a)+(o)))
#define ADDR_PUSH(a,s)  ((void*)((a)=(void*)(((char*)(a))-(s))))

#define MIN(a,b)    ((a)<=(b)?(a):(b))
#define MAX(a,b)    ((a)>=(b)?(a):(b))
#define MIN3(a,b,c)    MIN(a,MIN(b,c))
#define MAX3(a,b,c)    MAX(a,MAX(b,c))
#define POW2(v)     ((v) != 0 && ((v) & ((v)-1)) == 0)

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define __AT__  __FILE__ ":" TOSTRING(__LINE__)

static inline int POW2_UP(int val)
{
    if (val == 0 || POW2(val))
        return val;
    --val;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    return val + 1;
}

#define VERS32(mj,mn,pt) ( (((mj) & 0x3FF) << 20) | (((mn) & 0x3FF) << 8) | ((pt) & 0xFF) )
#define VERS32_MJ(v) (((v) >> 20) & 0x3FF)
#define VERS32_MN(v) (((v) >> 8) & 0x3FF)
#define VERS32_PT(v) ((v) & 0xFF)

#ifndef KORA_PRT
#  define _PRT(p)  p
#else
#  define _PRT(p)  p ## _p
#endif

#endif  /* _KORA_MCRS_H */
