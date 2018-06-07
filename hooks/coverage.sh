#!/bin/bash
#      This file is part of the KoraOS project.
#  Copyright (C) 2015  <Fabien Bavent>
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

SCRIPT_DIR=`dirname $BASH_SOURCE{0}`
SCRIPT_HOME=`readlink -f $SCRIPT_DIR/..`

LCOV="$SCRIPT_HOME/check.lcov"
lcov --rc lcov_branch_coverage=1 -c --directory $SCRIPT_HOME -b $SCRIPT_HOME -o $LCOV
sed -e '/SF:\/usr.*/,/end_of_record/d' -e '/SF:.*\/src\/tests\/.*/,/end_of_record/d' -i $LCOV
genhtml --rc lcov_branch_coverage=1 -o coverage $LCOV

# ./hooks/coverage.sh | grep '  lines......: [0-9.]*% ([0-9]* of [0-9]* lines)'
