# Ping testing
# Scenari: I connect my computer on a new network 
# and then send a ping address to the gateway  
ERROR ON

node gateway 1
ip4_config gateway:eth:1 dhcp-server
link lan gateway:eth:1
tempo
node alice 1
link lan alice:eth:1
tempo
ping alice 192.168.0.1

# Cleaning
unlink lan gateway:eth:1
unlink lan alice:eth:1
kill alice
kill gateway

# restart
