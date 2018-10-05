#!/bin/sh
set -x

# ip link add br0 type bridge
# ip link set enp3s0 master br0

switch=br0

if [ -n "$1" ];then
        #tunctl -u `whoami` -t $1
        ip tuntap add $1 mode tap user `whoami`
        ip link set $1 up
        sleep 0.5s
        #brctl addif $switch $1
        ip link set $1 master $switch
        exit 0
else
        echo "Error: no interface specified"
        exit 1
fi


# generate a random mac address for the qemu nic
macaddress ='DE:AD:BE:EF:%02X:%02X\n' $((RANDOM%256)) $((RANDOM%256))
print $macaddress

qemu-system-x86_64 -cdrom KoraOs.iso -device e1000,netdev=net0,mac=$macaddress -netdev tap,id=net0

