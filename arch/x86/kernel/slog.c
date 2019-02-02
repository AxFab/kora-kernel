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
#include <kernel/core.h>
#include <kernel/files.h>

void csl_early_init();
int csl_write(inode_t *ino, const char *buf, int len);

void com_early_init();
int com_output(int no, const char *buf, int len);


tty_t *slog;
int tty_write(tty_t *tty, const char *buf, int len);

void kwrite(const char *buf, int len)
{
    com_output(0, buf, len);
    if (slog != NULL)
        tty_write(slog, buf, len);
    // else
    //     csl_write(NULL, buf, len);
}

uint64_t cpu_clock()
{
    return 0;
}

time_t rtc_time();
void pit_interval(int hz);

time_t cpu_time()
{
    pit_interval(HZ);
    return rtc_time();
}
