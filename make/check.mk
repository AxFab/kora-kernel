#      This file is part of the KoraOS project.
#  Copyright (C) 2015-2021  <Fabien Bavent>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

SED_LCOV  = -e '/SF:\/usr.*/,/end_of_record/d'
SED_LCOV += -e '/SF:.*\/tests\/.*/,/end_of_record/d'


check: $(patsubst %,val_%,$(CHECKS))

split_coverage: $(patsubst %,cov_%,$(CHECKS))

coverage.lcov: check
	$(V) lcov --rc lcov_branch_coverage=1 -c --directory . -b . -o coverage.lcov
	$(S) sed $(SED_LCOV) -i coverage.lcov

do_coverage:
	$(V) lcov --rc lcov_branch_coverage=1 -c --directory . -b . -o coverage.lcov
	$(S) sed $(SED_LCOV) -i coverage.lcov
	$(V) genhtml --rc lcov_branch_coverage=1 -o coverage coverage.lcov

coverage: coverage.lcov
	$(V) genhtml --rc lcov_branch_coverage=1 -o coverage coverage.lcov

%.lcov: $(bindir)/%
	$(S) find -name *.gcda | xargs -r rm
	$(V) $<
	$(V) lcov --rc lcov_branch_coverage=1 -c --directory . -b . -o $@ >/dev/null
	$(S) sed $(SED_LCOV) -i $@

cov_%: %.lcov
	$(V) genhtml --rc lcov_branch_coverage=1 -o $@ $< >/dev/null

val_%: $(bindir)/%
	$(V) valgrind --error-exitcode=9 --leak-check=full --show-leak-kinds=all $< 2>&1 | tee $@

.PHONY: check coverage
