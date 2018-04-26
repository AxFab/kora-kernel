/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#ifndef _STDARG_H
#define _STDARG_H 1

#define _SIZEOF(n)      (((sizeof(n))+(sizeof(void*)-1))&(~(sizeof(void*)-1)))

/** The type va_list is defined for variables used to traverse the list.
 * The object ap may be passed as an argument to another function; if that
 * function invokes the va_arg() macro with parameter ap, the value of ap in
 * the calling function is indeterminate and must be passed to the va_end()
 * macro prior to any further reference to ap. The parameter argN is the
 * identifier of the rightmost parameter in the variable parameter list in the
 * function definition (the one just before the , ...). If the parameter argN
 * is declared with the register storage class, with a function type or array
 * type, or with a type that is not compatible with the type that results after
 * application of the default argument promotions, the behaviour is undefined.
*/
typedef char* va_list;

/** The va_start() macro is invoked to initialise ap to the beginning of the
 * list before any calls to va_arg(). */
#define va_start(ap,v)  ((ap) = (va_list)(&(v)) + _SIZEOF(v))

/** The va_arg() macro will return the next argument in the list pointed to by
 * ap. Each invocation of va_arg() modifies ap so that the values of successive
 * arguments are returned in turn. The type parameter is the type the argument
 * is expected to be. This is the type name specified such that the type of a
 * pointer to an object that has the specified type can be obtained simply by
 * suffixing a * to type. Different types can be mixed, but it is up to the
 * routine to know what type of argument is expected. */
#define va_arg(ap,t)    (*(t*) (((ap) += _SIZEOF(t)) - _SIZEOF(t)))

/** The va_end() macro is used to clean up; it invalidates ap for use
 * (unless va_start() is invoked again).  */
#define va_end(ap)      ((ap) = (va_list)0)

#define va_copy(ap,l)      ((ap) = l)

#endif /* STDARG_H__ */
