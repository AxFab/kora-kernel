# ----------------------------------------------------------------------------
# Create and execute tasks
START kloader.1
START kloader.2
TICK 5 1
START net_demon
TICK 2 2
SPAWN launcher
TICK 3 3
SPAWN job5
TICK 0 5
WAIT @w5
TICK 3 2
STOP
SHOW 0
TICK 0 1
WAIT @w1
TICK 0 3
WAIT @w3
TICK 0 4
WAIT @w4
SHOW 0


# ----------------------------------------------------------------------------
# Tasks on pause

# ----------------------------------------------------------------------------
# Threads

# ----------------------------------------------------------------------------
# Forks (later)

# ----------------------------------------------------------------------------
# Signals

# ----------------------------------------------------------------------------
# Exceptions and IRQ

# ----------------------------------------------------------------------------
# Keep track of CPU time

# ----------------------------------------------------------------------------
# Ressources

# ----------------------------------------------------------------------------
# System time

# ----------------------------------------------------------------------------
# Adjust time






