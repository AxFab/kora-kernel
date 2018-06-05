#!/bin/bash

lcov --rc lcov_branch_coverage=1 -c --directory . -b . -o check.lcov
sed -e '/SF:\/usr.*/,/end_of_record/d' -e '/SF:.*\/src\/tests\/.*/,/end_of_record/d' -i check.lcov
genhtml --rc lcov_branch_coverage=1 -o coverage check.lcov

# ./hooks/coverage.sh | grep '  lines......: [0-9.]*% ([0-9]* of [0-9]* lines)'
