# Network test

ERROR ON

NODE alice 1
NODE bob 1
IP4_CONFIG alice:eth:1 ip=192.168.0.1
IP4_CONFIG bob:eth:1 ip=192.168.0.2
LINK @nat0 alice:eth:1
LINK @nat0 bob:eth:1
TEMPO

SOCKET alice @as0 udp/ip4
EXPECT NONULL @as0
BIND @as0 0.0.0.0:7003
# ACCEPT @as0 @ac0 3

SOCKET bob @bc0 udp/ip4
EXPECT NONULL @bc0
CONNECT @bc0 192.168.0.1:7003

TEXT @bf0 "Hello, I'm Bob\n"
SEND @bc0 @bf0
TEMPO
ACCEPT @as0 @ac1 3
EXPECT NONULL @ac1
RECV @ac1 @bf1
EXPECT EQ @bf0 @bf1

CLOSE @ac1
CLOSE @bc0
CLOSE @as0

UNLINK @nat0 alice:eth:1
UNLINK @nat0 bob:eth:1
KILL alice
KILL bob

RMBUF @bf0
RMBUF @bf1

QUIT


# Test ping

#NODE gateway 2
#NODE alice 1

#IP4_CONFIG gateway:eth:1 ip=192.168.0.1
#IP4_CONFIG gateway:eth:1 dhcp-server
#TEMPO

#LINK @nat0 gateway:eth:1
#LINK @nat0 alice:eth:1

#TEMPO

#TEXT @bf2 "ABCD"
#PING alice 192.168.0.1 @bf2
#TEMPO

# Check received ping !!

