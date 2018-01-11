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
#include <skc/rwlock.h>
#include <stdlib.h>
#include <check.h>

START_TEST(test_rwlock_001)
{
  rwlock_t lock;
  rwlock_init(&lock);
  ck_assert(rwlock_rdtrylock(&lock) == 0);//should ok
  ck_assert(rwlock_wrtrylock(&lock) == BUSY);//should fail
  ck_assert(!rwlock_wrlocked(&lock));
  rwlock_rdunlock(&lock);
  ck_assert(rwlock_wrtrylock(&lock) == 0);//should ok
  ck_assert(rwlock_wrlocked(&lock));
  ck_assert(rwlock_rdtrylock(&lock) == BUSY);//should fail
  ck_assert(rwlock_wrtrylock(&lock) == BUSY);//should fail
  rwlock_wrunlock(&lock);
  ck_assert(rwlock_wrtrylock(&lock) == 0);//should ok
  ck_assert(rwlock_wrlocked(&lock));
  // ck_assert(rwlock_wrlock(&lock) == BUSY);//should wait
  rwlock_wrunlock(&lock);

} END_TEST

START_TEST(test_rwlock_002)
{
  rwlock_t lock;
  rwlock_init(&lock);
  rwlock_wrlock(&lock);
  ck_assert(rwlock_wrlocked(&lock));
  ck_assert(rwlock_rdtrylock(&lock) == BUSY);
  rwlock_wrunlock(&lock);
  ck_assert(!rwlock_wrlocked(&lock));
  rwlock_rdlock(&lock);
  rwlock_rdlock(&lock);
  ck_assert(rwlock_wrtrylock(&lock) == BUSY);//should fail
  ck_assert(!rwlock_wrlocked(&lock));
  rwlock_rdunlock(&lock);
  ck_assert(rwlock_wrtrylock(&lock) == BUSY);//should fail
  ck_assert(!rwlock_wrlocked(&lock));
  rwlock_rdunlock(&lock);
  ck_assert(rwlock_wrtrylock(&lock) == 0);//should ok
  ck_assert(rwlock_wrlocked(&lock));
  rwlock_wrunlock(&lock);

} END_TEST

START_TEST(test_rwlock_003)
{
  rwlock_t lock;
  rwlock_init(&lock);
  rwlock_rdlock(&lock);
  ck_assert(!rwlock_wrlocked(&lock));
  ck_assert(rwlock_wrtrylock(&lock) == BUSY);//should fail
  ck_assert(!rwlock_wrlocked(&lock));
  ck_assert(rwlock_upgrade(&lock) == 0);
  ck_assert(rwlock_wrlocked(&lock));
  ck_assert(rwlock_wrtrylock(&lock) == BUSY);//should fail
  rwlock_wrunlock(&lock);
  ck_assert(rwlock_wrtrylock(&lock) == 0);
  ck_assert(rwlock_upgrade(&lock) == BUSY);
  rwlock_wrunlock(&lock);

} END_TEST

void fixture_rwlock(Suite *s)
{
  TCase *tc = tcase_create("Read-write Locks");
  tcase_add_test(tc, test_rwlock_001);
  tcase_add_test(tc, test_rwlock_002);
  tcase_add_test(tc, test_rwlock_003);
  suite_add_tcase(s, tc);
}
