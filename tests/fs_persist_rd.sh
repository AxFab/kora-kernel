
ERROR ON

LS .
LS ./Kah
PWD

# Check folder content
STAT Kah/Bar.txt REG 0644
SIZE Kah/Bar.txt 18309

STAT Kah/Lorem.txt REG 0644
SIZE Kah/Lorem.txt 18309

STAT Kah/Foo.blank REG 0644
SIZE Kah/Foo.blank 0

STAT Kah/Bnz DIR 0755
STAT Kah/Lrz DIR 0755
STAT Kah/Sto DIR 0755
STAT Kah/Zig DIR 0755


# Folder Kah/Bnz/ contains files named sample_xx.txt


# Path search
STAT Kah/Lrz/Foo.txt REG 0644
SIZE Kah/Lrz/Foo.txt 18309
STAT Kah/Lrz/../Bar.txt REG 0644
SIZE Kah/./Bar.txt 18309
ERROR ENOENT
STAT Kah/Bar/Foo.txt
ERROR ON
LS Kah/Lrz 1


# Read
CRC32 Kah/Bar.txt 3c8fb882
CRC32 Kah/Lorem.txt 3c8fb882
CRC32 Kah/Lrz/Foo.txt 3c8fb882

