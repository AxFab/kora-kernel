#!/bin/bash


set -eu

make clean
export NODEPS=y
export ADD_C11=y
make cli-mem
make cli-net
make cli-snd
make cli-tsk
make cli-vfs

cd tests
 ../bin/cli-mem mm_start.sh # Not complete (lorem_sm.txt)
../bin/cli-net net_ping0.sh
../bin/cli-net net_ip0.sh
../bin/cli-net net_udp0.sh
../bin/cli-snd <(echo '# empty')
../bin/cli-tsk tsk_main.sh # NOT WORKING
../bin/cli-vfs fs_ext2.sh # Not complete (lorem_sm.txt)
../bin/cli-vfs fs_isofs.sh
../bin/cli-vfs fs_tarfs.sh
../bin/cli-vfs fs_main.sh

cd ../
make do_coverage
