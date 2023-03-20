#!/usr/bin/env cli_fs
# ---------------------------------------------------------------------------
ERROR ON

# ---------------------------------------------------------------------------
# Write a dummy file, and read checksum
MKDIR Foo

IMG_OPEN lorem_sm.txt lorem_sm
DD /dev/lorem_sm Foo/Bar 32
CRC32 Foo/Bar 3c8fb882
UNLINK Foo/Bar

IMG_OPEN lorem_md.txt lorem_md
DD /dev/lorem_md Foo/Gap 32
CRC32 Foo/Gap b69a152f
UNLINK Foo/Gap

# DD /dev/lorem_md Foo/Bar 32 5581184
# CRC32 Foo/Bar
# UNLINK Foo/Bar

RMDIR Foo

# ---------------------------------------------------------------------------
# Create a file with a hole, check size on disk, read is correct

# ---------------------------------------------------------------------------
# Write will extend the file as needed

# ---------------------------------------------------------------------------
# Write will update mtime but not ctime

# ---------------------------------------------------------------------------
# Read will update atime

# ---------------------------------------------------------------------------
# Create a really large file, check space left and integrity

# ---------------------------------------------------------------------------
# Create many file, then extend file to create partition, verify checksum

# ---------------------------------------------------------------------------
# Test randomize writing

# ---------------------------------------------------------------------------
# Test randomize reading


