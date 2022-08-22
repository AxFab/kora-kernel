# Udp simple exchange testing
# Scenari: I connect to a UDP service and excahnge some packets
node server 1
ip4_config server:eth:1 ip=192.168.0.1
link lan server:eth:1

node alice 1
ip4_config alice:eth:1 ip=192.168.0.4
link lan alice:eth:1

socket server sks udp
bind sks 0.0.0.0:7003
accept sks sk0 0

socket alice ska udp
connect ska 192.168.0.1:7003

text ba "Hello, I'm Alice\n"
send ska ba

accept sks sk0 50
recv sk0 bs
cmp ba bs
text bs "Hello Alice\n"
send sk0 bs

recv ska ba
print ba
close ska

# Create a second client

node bob 1
ip4_config bob:eth:1 ip=192.168.0.5
link lan bob:eth:1

text ba "Nice to meet you!\n"
send ska ba

socket bob skb udp
connect skb 192.168.0.1:7003

text bb "Hello, I'm Bob\n"
send skb bb

accept sks sk1 500

recv sk0 bs
print bs

recv sk1 bs
print bs

text bs "Hello Bob\n"
send sk1 bs

text bs "Is someone there?\n"
send sks bs

