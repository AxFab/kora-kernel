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
#define _tcsnlen  _PFX(nlen)
#define _tcsncat  _PFX(ncat)
#define _tcsncpy  _PFX(ncpy)
#define _tcsnset  _PFX(nset)
#define _tcspbrk  _PFX(pbrk)
#define _tcsrchr  _PFX(rchr)
#define _tcsrspn  _PFX(spn)
#define _tcsstr  _PFX(str)
#define _tcstok  _PFX(tok)
#define _tcstok_r  _PFX(tok_r)

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
#define _tprintf_l  _NFX(,printf_l)
#define _tvprintf  _NFX(v,printf)
#define _tvprintf_l  _NFX(v,printf_l)
#define _tfprintf  _NFX(f,printf)
#define _tfprintf_l  _NFX(f,printf_l)
#define _tvfprintf  _NFX(vf,printf)
#define _tvfprintf_l  _NFX(vf,printf_l)
#define _tdprintf  _NFX(d,printf)
#define _tdprintf_l  _NFX(d,printf_l)
#define _tvdprintf  _NFX(vd,printf)
#define _tvdprintf_l  _NFX(vd,printf_l)
#define _tsprintf  _NFX(s,printf)
#define _tsprintf_l  _NFX(s,printf_l)
#define _tvsprintf  _NFX(vs,printf)
#define _tvsprintf_l  _NFX(vs,printf_l)
#define _tsnprintf  _NFX(sn,printf)
#define _tsnprintf_l  _NFX(sn,printf_l)
#define _tvsnprintf  _NFX(vsn,printf)
#define _tvsnprintf_l  _NFX(vsn,printf_l)
#define _tscanf  _NFX(,scanf)
#define _tscanf_l  _NFX(,scanf_l)
#define _tvscanf  _NFX(v,scanf)
#define _tvscanf_l  _NFX(v,scanf_l)
#define _tfscanf  _NFX(f,scanf)
#define _tfscanf_l  _NFX(f,scanf_l)
#define _tvfscanf  _NFX(vf,scanf)
#define _tvfscanf_l  _NFX(vf,scanf_l)
#define _tdscanf  _NFX(d,scanf)
#define _tdscanf_l  _NFX(d,scanf_l)
#define _tvdscanf  _NFX(vd,scanf)
#define _tvdscanf_l  _NFX(vd,scanf_l)
#define _tsscanf  _NFX(s,scanf)
#define _tsscanf_l  _NFX(s,scanf_l)
#define _tvsscanf  _NFX(vs,scanf)
#define _tvsscanf_l  _NFX(vs,scanf_l)
#define _tsnscanf  _NFX(sn,scanf)
#define _tsnscanf_l  _NFX(sn,scanf_l)
#define _tvsnscanf  _NFX(vsn,scanf)
#define _tvsnscanf_l  _NFX(vsn,scanf_l)


/* Unformatted i/o */
#define _fgettc  _NFX(fget,c)
#define _fgettc_nolock  _NFX(fget,c_nolock)
#define _fgetts  _NFX(fget,s)
#define _fputtc  _NFX(fput,c)
#define _fputtc_nolock  _NFX(fput,c_nolock)
#define _fputts  _NFX(fput,s)
#define _gettc  _NFX(get,c)
#define _gettc_nolock  _NFX(get,c_nolock)
#define _getts  _NFX(get,s)
#define _puttc  _NFX(put,c)
#define _puttc_nolock  _NFX(put,c_nolock)
#define _putts  _NFX(put,s)
#define _ungettc  _NFX(unget,c)
#define _ungettc_nolock  _NFX(unget,c_nolock)


/* String conversion functions */
#define _tcstod  _NFX(,tod)
#define _tcstof  _NFX(,tof)
#define _tcstold  _NFX(,told)
#define _tcstoi  _NFX(,toi)
#define _tcstou  _NFX(,tou)
#define _tcstol  _NFX(,tol)
#define _tcstoul  _NFX(,toul)
#define _tcstoll  _NFX(,toll)
#define _tcstoull  _NFX(,toull)
#define _tcstoi64  _NFX(,toi64)
#define _tcstou64  _NFX(,tou64)
#define _tcstoimax  _NFX(,toimax)
#define _tcstoumax  _NFX(,toumax)

#define _itot  _AFX(ito,)
#define _ltot  _AFX(lto,)
#define _ultot  _AFX(ulto,)
#define _ttoi  _AFX(,toi)
#define _ttol  _AFX(,tol)
#define _ttoul  _AFX(,toul)


/* Time functions */



/* Ctype functions */
#define _istalnum  _NFX(is,alnum)
#define _istalnum_l  _NFX(is,alnum_l)
#define _istalpha  _NFX(is,alpha)
#define _istalpha_l  _NFX(is,alpha_l)
#define _istcntrl  _NFX(is,cntrl)
#define _istcntrl_l  _NFX(is,cntrl_l)
#define _istdigit  _NFX(is,digit)
#define _istdigit_l  _NFX(is,digit_l)
#define _istgraph  _NFX(is,graph)
#define _istgraph_l  _NFX(is,graph_l)
#define _istlower  _NFX(is,lower)
#define _istlower_l  _NFX(is,lower_l)
#define _istprint  _NFX(is,print)
#define _istprint_l  _NFX(is,print_l)
#define _istpunct  _NFX(is,punct)
#define _istpunct_l  _NFX(is,punct_l)
#define _istspace  _NFX(is,space)
#define _istspace_l  _NFX(is,space_l)
#define _istupper  _NFX(is,upper)
#define _istupper_l  _NFX(is,upper_l)
#define _istxdigit  _NFX(is,xdigit)
#define _istxdigit_l  _NFX(is,xdigit_l)

#define _totupper  _NFX(to,upper)
#define _totupper_l  _NFX(to,upper_l)
#define _totlower  _NFX(to,lower)
#define _totlower_l  _NFX(to,lower_l) 
 

#endif  /* _KORA_TCHAR_H */
