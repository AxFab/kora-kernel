/*
 *      This file is part of the SmokeOS project.
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
#include <kernel/core.h>
#include <kernel/memory.h>
#include <skc/mcrs.h>
#include <skc/iofile.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
int k_errno;

void __perror_fail(int err, const char * file, int line, const char *msg)
{
  kprintf(-1, "ERROR] Process fails (%d) at %s:%d -- %s\n", err, file, line, msg);
}

void __assert_fail(const char *expr, const char * file, int line) {
  kprintf(-1, "ERROR] Assertion (%s) at %s:%d -- %s\n", expr, file, line);
}

int *__errno_location()
{
  return &k_errno;
}

int *__ctype_b_loc()
{
  return NULL;
}

void *heap_map(size_t length)
{
  return kmap(length, NULL, 0, VMA_FG_HEAP);
}

void heap_unmap(void* address, size_t length)
{
  kunmap(address, length);
}

void abort()
{
  for (;;);
}

int snprintf(char*, int, const char*, ...);
/* Store in a temporary buffer a size in bytes in a human-friendly format. */
char *sztoa(size_t number)
{
  static char sz_format[20];
  static const char *prefix[] = { "bs", "Kb", "Mb", "Gb", "Tb", "Pb", "Eb" };
  int k = 0;
  int rest = 0;

  while (number > 1024) {
    k++;
    rest = number & (1024 - 1);
    number /= 1024;
  };

  if (k == 0) {
    snprintf (sz_format, 20, "%d bytes", (int)number);

  } else if (number < 10) {
    float value = (rest / 1024.0f) * 100;
    snprintf (sz_format, 20, "%1d.%02d %s", (int)number, (int)value, prefix[k]);

  } else if (number < 100) {
    float value = (rest / 1024.0f) * 10;
    snprintf (sz_format, 20, "%2d.%01d %s", (int)number, (int)value, prefix[k]);

  } else {
    // float value = (rest / 1024.0f) + number;
    //snprintf (sz_format, 20, "%.3f %s", (float)value, prefix[k]);
    snprintf (sz_format, 20, "%4d %s", (int)number, prefix[k]);
  }

  return sz_format;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
FILE *klogs = NULL;

int vfprintf(FILE *fp, const char *str, va_list ap);

void kprintf(int log, const char *msg, ...)
{
  va_list ap;
  va_start(ap, msg);
  if (klogs != NULL) {
    vfprintf(klogs, msg, ap);
  }
  va_end(ap);
}

void *kalloc(size_t size)
{
  void *ptr = malloc(size);
  memset(ptr, 0, size);
  return ptr;
}

void kfree(void *ptr)
{
  size_t size = 8; // TODO szofalloc(ptr);
  memset(ptr, 0, size);
  free(ptr);
}

void *kmap(size_t length, inode_t *ino, off_t offset, int flags)
{
  flags &= ~(VMA_RIGHTS << 4);
  flags |= (flags & VMA_RIGHTS) << 4;
  return memory_map(kMMU.kspace, 0, length, ino, offset, flags);
}

void kunmap(void *address, size_t length)
{
  memory_flag(kMMU.kspace, (size_t)address, length, VMA_DEAD);
}

void kpanic(const char *msg)
{
  kprintf(0, "\033[31m;Kernel panic: %s \033[0m;\n", msg);
  abort();
}

void kclock(struct timespec *ts)
{
  ts->tv_sec = 0;
  ts->tv_nsec = 0;
}
