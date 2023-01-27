#!/bin/bash

set -eu

make clean
export NODEPS=y
make cli-mem
make cli-net
make cli-snd
# make cli-tsk
make cli-vfs

cd tests
../bin/cli-mem mm_start.sh
../bin/cli-net net_ping0.sh
../bin/cli-net net_ip0.sh
# # ../bin/cli-net net_udp0.sh
../bin/cli-snd <(echo '# empty')
# ../bin/cli-tsk tsk_main.sh
# ../bin/cli-vfs fs_ext2.sh
# ../bin/cli-vfs fs_isofs.sh
# ../bin/cli-vfs fs_tarfs.sh
# ../bin/cli-vfs fs_main.sh

cd ../
make do_coverage
