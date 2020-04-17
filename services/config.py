# ------------------------------------------------------------------------------
# SCP section
# ------------------------------------------------------------------------------
# Sleep time (in fraction of second) between checking new requests
tick = 0.1
scpHost = "192.168.1.107"
scpPort = 9999


# ------------------------------------------------------------------------------
# Logging
# ------------------------------------------------------------------------------
# Root level prefix of the sender that will be used by logger
rootLoggerName = 'VAS'
# Directory where log files will be created
logDir = './logs'
# Host where logs will be sent to
logServerHostname = 'localhost'


# ------------------------------------------------------------------------------
# MissedCall
# ------------------------------------------------------------------------------
serviceKey=256
needOnlyOne=1


unittestSCPCgPN = '+3741111111'
unittestSCPRdPN = '95989136'
unittestSCPReason = 4096
unittestSDPAddrSrc = '95989136'
unittestSDPServiceKey = 256
