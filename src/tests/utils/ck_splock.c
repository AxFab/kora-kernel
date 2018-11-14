/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <kora/splock.h>
#include <stdlib.h>
#include "../check.h"


START_TEST(test_splock_001)
{
    splock_t lock;
    splock_init(&lock);
    splock_lock(&lock);
    ck_assert(splock_locked(&lock));
    splock_unlock(&lock);
    ck_assert(!splock_locked(&lock));

}
END_TEST

START_TEST(test_splock_002)
{
    splock_t lock;
    splock_init(&lock);
    ck_assert(splock_trylock(&lock) == true);
    ck_assert(splock_locked(&lock));
    ck_assert(splock_trylock(&lock) == false);
    splock_unlock(&lock);
    ck_assert(!splock_locked(&lock));
    cpu_relax();

}
END_TEST

void fixture_splock(Suite *s)
{
    TCase *tc = tcase_create("Spinlocks");
    tcase_add_test(tc, test_splock_001);
    tcase_add_test(tc, test_splock_002);
    suite_add_tcase(s, tc);
}
