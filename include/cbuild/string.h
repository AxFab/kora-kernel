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

/* Searches the first len bytes of array str for character c. */
_TVOID *_SFM(chr)(const _TVOID *str, int c, size_t len);
/* Compares two blocks of signed chars. */
int _SFM(cmp)(const _TVOID *s1, const _TVOID *s2, size_t len);
/* Copies a block of len bytes from src to dest. */
_TVOID *_SFM(cpy)(_TVOID *dest, const _TVOID *src, size_t len);
/* Copies a block of len bytes from src to dest, with possibility of overlaping of source and destination block. */
_TVOID *_SFM(move)(_TVOID *dest, const _TVOID *src, size_t len);
/* Sets num bytes of buffer to byte c. */
_TVOID *_SFM(set)(_TVOID *buffer, int c, size_t num);


/* Appends src to dest. */
_TCHAR *_SFX(cat)(_TCHAR *dest, const _TCHAR *src);
/* Finds c in str. */
_TCHAR *_SFX(chr)(const _TCHAR *str, int c);
/* Compares one string to another. */
int _SFX(cmp)(const _TCHAR *s1, const _TCHAR *s2);
/* Copies string src to dest. */
_TCHAR *_SFX(cpy)(_TCHAR *dest, const _TCHAR *src);
/* Scans a string. */
size_t _SFX(cspn)(const _TCHAR *s1, const _TCHAR *s2);
/* Performs case-insensitive string comparison. */
int _SFX(icmp)(const _TCHAR *s1, const _TCHAR *s2);
/* Calculates length of a string. */
size_t _SFX(len)(const _TCHAR *str);
/* Calculates length of a string. */
size_t _SFX(nlen)(const _TCHAR *str, size_t maxlen);
/* Appends at most maxlen characters of src to dest. */
_TCHAR *_SFX(ncat)(_TCHAR *dest, const _TCHAR *src, size_t maxlen);
/* Compares at most maxlen characters of one string to another. */
int _SFX(ncmp)(const _TCHAR *s1, const _TCHAR *s2, size_t maxlen);
/* Copies at most maxlen characters of src to dest. */
_TCHAR *_SFX(ncpy)(_TCHAR *dest, const _TCHAR *src, size_t maxlen);
/* Scans one string for the first occurrence of any character that's in a second string. */
_TCHAR *_SFX(pbrk)(const _TCHAR *s1, const _TCHAR *s2);
/* Finds the last occurrence of c in str. */
_TCHAR *_SFX(rchr)(const _TCHAR *str, int c);
/* Scans a string for a segment that is a subset of a set of characters. */
size_t _SFX(spn)(const _TCHAR *s1, const _TCHAR *s2);
/* Finds the first occurrence of a substring in another string. */
_TCHAR *_SFX(str)(const _TCHAR *str, const _TCHAR *substr);
/* Scans s1 for the first token not contained in s2. */
_TCHAR *_SFX(tok)(_TCHAR *s1, const _TCHAR *s2);
/* duplicate a string  */
_TCHAR *_SFX(dup)(const _TCHAR *str);
/* duplicate a string at most maxlen characters */
_TCHAR *_SFX(ndup)(const _TCHAR *str, size_t maxlen);
/* Convert a string to lowercase. */
_TCHAR *_SFX(lwr)(_TCHAR *str);
/* Convert a string to uppercase. */
_TCHAR *_SFX(upr)(_TCHAR *string);
/* Compare characters of two strings without regard to case. */
int _SFX(nicmp)(const _TCHAR *s1, const _TCHAR *s2, size_t maxlen);
/* Reverse characters of a string. */
_TCHAR *_SFX(rev)(_TCHAR *string);
/* Set characters of a string to a character. */
_TCHAR *_SFX(set)(_TCHAR *str, int c);
/* Initialize characters of a string to a given format. */
_TCHAR *_SFX(nset)(_TCHAR *str, int c, size_t maxlen);

/* Scans s1 for the first token not contained in s2. */
_TCHAR *_SFX(tok_r)(_TCHAR *s1, const _TCHAR *s2, _TCHAR **rent);

/* Compare strings using locale-specific information. */
int _SFX(coll)(const _TCHAR *s1, const _TCHAR *s2);
int _SFX(icoll)(const _TCHAR *s1, const _TCHAR *s2);
int _SFX(ncoll)(const _TCHAR *s1, const _TCHAR *s2, size_t maxlen);
int _SFX(nicoll)(const _TCHAR *s1, const _TCHAR *s2, size_t maxlen);

/* String transformation */
size_t _SFX(xfrm)(char *dest, const char *src, size_t n);
