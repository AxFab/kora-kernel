# Udp simple exchange testing
# Scenari: I CONNECT to a UDP service and excahnge some packets
NODE server 1
IP4_CONFIG server:eth:1 ip=192.168.0.1
LINK lan server:eth:1

NODE alice 1
IP4_CONFIG alice:eth:1 ip=192.168.0.4
LINK lan alice:eth:1

SOCKET server @sks udp
BIND @sks 0.0.0.0:7003
ACCEPT @sks @sk0 0

SOCKET alice @ska udp
CONNECT @ska 192.168.0.1:7003

TEXT @ba "Hello, I'm Alice\n"
SEND @ska @ba

ACCEPT @sks @sk0 50
RECV @sk0 @bs
EXPECT EQ @ba @bs
TEXT @bs "Hello Alice\n"
SEND @sk0 @bs

RECV @ska @ba
# PRINT @ba
CLOSE @ska

# Create a second client

NODE @bob 1
IP4_CONFIG bob:eth:1 ip=192.168.0.5
LINK lan bob:eth:1

TEXT @ba "Nice to meet you!\n"
SEND @ska @ba

SOCKET @bob @skb udp
CONNECT @skb 192.168.0.1:7003

TEXT @bb "Hello, I'm Bob\n"
SEND @skb @bb

ACCEPT @sks @sk1 500

RECV @sk0 @bs
# PRINT @bs

RECV @sk1 @bs
# PRINT @bs

TEXT @bs "Hello Bob\n"
SEND @sk1 @bs

TEXT @bs "Is someone there?\n"
SEND @sks @bs

