
SED_LCOV  = -e '/SF:\/usr.*/,/end_of_record/d'
SED_LCOV += -e '/SF:.*\/src\/tests\/.*/,/end_of_record/d'


check: $(patsubst %,val_%,$(CHECKS))

split_coverage: $(patsubst %,cov_%,$(CHECKS))

coverage.lcov: check
	$(V) lcov --rc lcov_branch_coverage=1 -c --directory . -b . -o coverage.lcov
	$(S) sed $(SED_LCOV) -i coverage.lcov

coverage: coverage.lcov
	genhtml --rc lcov_branch_coverage=1 -o coverage coverage.lcov

.PHONY: check


%.lcov: $(bindir)/%
	$(S) find -name *.gcda | xargs -r rm
	$(V) $<
	$(V) lcov --rc lcov_branch_coverage=1 -c --directory . -b . -o $@ >/dev/null
	$(S) sed $(SED_LCOV) -i $@

cov_%: %.lcov
	$(V) genhtml --rc lcov_branch_coverage=1 -o $@ $< >/dev/null

val_%: $(bindir)/%
	$(V) valgrind --error-exitcode=9 --leak-check=full --show-leak-kinds=all $< 2>&1 | tee $@


# lcov --rc lcov_branch_coverage=1 -c --directory . -b . -o src.lcov
# sed -e '/SF:\/usr.*/,/end_of_record/d' -e '/SF:.*\/src\/tests\/.*/,/end_of_record/d' -i src.lcov
# genhtml --rc lcov_branch_coverage=1 -o cover src.lcov
