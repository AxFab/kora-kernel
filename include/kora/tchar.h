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
#ifndef _KORA_TCHAR_H
#define _KORA_TCHAR_H 1

#include <locale.h>

#if defined _MBCS

typedef char TCHAR;
#include <mbstring.h>
#define _PFX(s)  mbs ## s
#define _NFX(p,s)  p ## mb ## s
#define _AFX(p,s)  p ## mb ## s

#define _tclen(c)  mbclen(c)
#define _tccpy(d,s)  mbccpy(&(d),&(s)) 
#define _tccmp(a,b)  mbccmp(&(a),&(b)) 

#elif defined _UNICODE

typedef wchar_t TCHAR;
#include <wchar.h>
#define _PFX(s)  wcs ## s
#define _NFX(p,s)  p ## w ## s
#define _AFX(p,s)  p ## w ## s

#define _tclen(c)  sizeof(wchar_t)
#define _tccpy(d,s)  ((d)=(s)) 
#define _tccmp(a,b)  ((a)==(b)) 

#else 

typedef char TCHAR;
#include <string.h>
#define _PFX(s)  str ## s
#define _NFX(p,s)  p ## s
#define _AFX(p,s)  p ## a ## s

#define _tclen(c)  (1)
#define _tccpy(d,s)  ((d)=(s)) 
#define _tccmp(a,b)  ((a)==(b)) 

#endif  /* _MBCS || _UNICODE */

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* String functions */
#define _tcscat  _PFX(cat) 
#define _tcschr  _PFX(chr) 
#define _tcscpy  _PFX(cpy) 
#define _tcsdup  _PFX(dup) 
#define _tcsspn  _PFX(spn) 
#define _tcslen  _PFX(len) 
#define _tcsncat  _PFX(ncat) 
#define _tcsncpy  _PFX(ncpy) 
#define _tcspbrk  _PFX(pbrk) 
#define _tcsrchr  _PFX(rchr) 
#define _tcsstr  _PFX(str) 
#define _tcstok  _PFX(tok) 
#define _tcstok  _PFX(tok_r) 

#define _tcserror  _PFX(error) 
#define _tcserror_r  _PFX(error_r) 

#define _tcscmp  _PFX(cmp) 
#define _tcscmp_l  _PFX(cmp_l) 
#define _tcsicmp  _PFX(icmp) 
#define _tcsicmp_l  _PFX(icmp_l)
#define _tcsncmp  _PFX(ncmp) 
#define _tcsncmp_l  _PFX(ncmp_l) 
#define _tcsnicmp  _PFX(nicmp) 
#define _tcsnicmp_l  _PFX(nicmp_l) 

#define _tcscoll  _PFX(coll) 
#define _tcscoll_l  _PFX(coll_l) 
#define _tcsicoll  _PFX(icoll) 
#define _tcsicoll_l  _PFX(icoll_l)
#define _tcsncoll  _PFX(ncoll) 
#define _tcsncoll_l  _PFX(ncoll_l) 
#define _tcsnicoll  _PFX(nicoll) 
#define _tcsnicoll_l  _PFX(nicoll_l) 

#define _tcslwr  _PFX(lwr) 
#define _tcslwr_l  _PFX(lwr_l) 
#define _tcsnlwr  _PFX(nlwr) 
#define _tcsnlwr_l  _PFX(nlwr_l) 
#define _tcsupr  _PFX(upr) 
#define _tcsupr_l  _PFX(upr_l) 
#define _tcsnupr  _PFX(nupr) 
#define _tcsnupr_l  _PFX(nupr_l) 

#define _tcsfrm  _PFX(frm) 
#define _tcsfrm_l  _PFX(frm_l) 


/* Formatted i/o */
#define _tprintf  _NFX(,printf)
#define _tvprintf  _NFX(v,printf)
#define _tfprintf  _NFX(f,printf)
#define _tvfprintf  _NFX(vf,printf)
#define _tdprintf  _NFX(d,printf)
#define _tvdprintf  _NFX(vd,printf)
#define _tsprintf  _NFX(s,printf)
#define _tvsprintf  _NFX(vs,printf)
#define _tsnprintf  _NFX(sn,printf)
#define _tvsnprintf  _NFX(vsn,printf)


/* Unformatted i/o */


/* Ctype functions */



#endif  /* _KORA_TCHAR_H */
