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
#include <skc/splock.h>
#include <stdlib.h>
#include <check.h>

START_TEST(test_memop_001)
{
  const char *msg = "ABCDEFGHIJKLMNOP";
  char msg0[6];
  memset(msg0, 0x55, 6);
  ck_assert(msg0[0] == 0x55);
  ck_assert(msg0[5] == 0x55);

  memcpy(msg0, msg, 6);
  ck_assert(msg0[0] == 'A');
  ck_assert(msg0[5] == 'F');
  ck_assert(memcmp(msg0, msg, 6) == 0);

  memset(&msg0[3], 0xAA, 3);
  ck_assert(memcmp(msg0, msg, 6) != 0);

} END_TEST

void fixture_string(Suite *s)
{
  TCase *tc;

  tc = tcase_create("Memory operations");
  tcase_add_test(tc, test_memop_001);
  suite_add_tcase(s, tc);
}
