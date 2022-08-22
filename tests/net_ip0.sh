# Network test

# VAR nat0 NAT
# VAR as0 SOCK
# VAR ac0 SOCK
# VAR ac1 SOCK
# VAR bc0 SOCK
# VAR bf0 BUF
# VAR bf1 BUF

node alice 1
node bob 1
ip4_config alice:eth:1 ip=192.168.0.1
ip4_config bob:eth:1 ip=192.168.0.2
LINK nat0 alice:eth:1
LINK nat0 bob:eth:1
tempo

socket alice as0 udp/ip4
# EXPECT NONULL as0
BIND as0 0.0.0.0:7003
ACCEPT as0 ac0 3

socket bob bc0 udp/ip4
# EXPECT NONULL bc0
CONNECT bc0 192.168.0.1:7003

TEXT bf0 "Hello, I'm Bob\n"
WRITE bc0 bf0
tempo
ACCEPT as0 ac1 3
# EXPECT NONULL ac1
READ ac1 bf1
EXPECT EQ bf0 bf1




# Test ping

node gateway 2
node alice 1

ip4_config gateway:eth:1 ip=192.168.0.1
ip4_config gateway:eth:1 dhcp-server
tempo

LINK nat0 gateway:eth:1
LINK nat0 alice:eth:1

tempo

TEXT bf "ABCD"
PING alice 192.168.0.1 bf
tempo

# Check received ping !!

