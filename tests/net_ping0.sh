# Ping testing
# Scenari: I connect my computer on a new network 
# and then send a ping address to the gateway  
ERROR ON

# Initialize rooter
NODE gateway 1
IP4_CONFIG gateway:eth:1 dhcp-server
LINK lan gateway:eth:1
TEMPO

# Initialize client
NODE alice 1
LINK lan alice:eth:1
TEMPO

# Send ping request
PING alice 192.168.0.1

# Cleaning
UNLINK lan gateway:eth:1
UNLINK lan alice:eth:1
KILL alice
KILL gateway
QUIT

