#!/bin/bash

# A hook script to verify what is about to be committed.
# Called by "git commit" with no arguments.

if git rev-parse --verify HEAD >/dev/null 2>&1
then
	against=HEAD
else
	# Initial commit: diff against an empty tree object
	against=4b825dc642cb6eb9a060e54bf8d69288fbee4904
fi

# If you want to allow non-ASCII filenames set this variable to true.
allownonascii=$(git config --bool hooks.allownonascii)

# Redirect output to stderr.
exec 1>&2

# Cross platform projects tend to avoid non-ASCII filenames; prevent
# them from being added to the repository. We exploit the fact that the
# printable range starts at the space character and ends with tilde.
if
	# Note that the use of brackets around a tr range is ok here, (it's
	# even required, for portability to Solaris 10's /usr/bin/tr), since
	# the square bracket bytes happen to fall in the designated range.
	test $(git diff --cached --name-only --diff-filter=A -z $against |
	  LC_ALL=C tr -d '[ -~]\0' | wc -c) != 0
then
	echo 'Error: Attempt to add a non-ASCII file name.'
	exit 1
fi

# If there are whitespace errors, print the offending file names and fail.
exec git diff-index --check --cached $against --

# astyle --style=kr --indent=spaces=4 --indent-col1-comments --min-conditional-indent=1 --pad-oper --pad-header --unpad-paren --align-pointer=name --align-reference=name --break-one-line-headers --remove-brackets --convert-tabs --lineend=linux --mode=c -r "*.c" "*.h"

