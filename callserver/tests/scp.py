import logging
import sys
import time
import struct
import zmq

import scp_interface

# Configure logger
logger = logging.getLogger('SCP')
logger.setLevel(logging.DEBUG)
consoleHandler = logging.StreamHandler()
formatter = logging.Formatter('%(asctime)s %(levelname)-8s %(name)-24s %(message)s', datefmt='%Y/%m/%d %H:%M:%S')
consoleHandler.setFormatter(formatter)
logger.addHandler(consoleHandler)


def Process(data, receiver):
    if len(data) <= 8:
        logger.error("onRequestReceived : Malformed request")
        return

    requestID = struct.unpack('I', data[0:4])[0]
    sessionID = struct.unpack('I', data[4:8])[0]
    logger.error("onRequestReceived : requestID [%d]  sessionID [%d]" % (requestID, sessionID))
    
    # get event type and call corresponding handler
    if data[8] == scp_interface.CallServerEvents.GCEV_OFFERED:
        event = scp_interface.EvOffered()
        if not event.unpack(data[9:]):
            logger.error("onRequestReceived : Malformed request")
            return
        else:
            logger.debug("onRequestReceived : GCEV_OFFERED")
            logger.debug(event)

            # Drop the call
            response = scp_interface.CallServerCommand.dropCall(1234)
            logger.debug("onOffered : sending command DROP with reason %d" % 1234)
            receiver.send(response)

    else:
        logger.error("onRequestReceived : Unhandled event code %d" % data[0])



context = zmq.Context()

# Socket to receive messages on
#receiver = context.socket(zmq.PULL)
#receiver.bind("tcp://*:5558")
receiver = context.socket(zmq.REP)
receiver.connect("tcp://localhost:5558")

# Socket to send messages to
#sender = context.socket(zmq.PUSH)
#sender.bind("tcp://*:5557")
sender = context.socket(zmq.REQ)
sender.connect("tcp://localhost:5557")

poller = zmq.Poller()
#poller.register(sender, zmq.POLLOUT)
poller.register(receiver, zmq.POLLIN)

requests_cnt = 0

# Process tasks forever
while True:
    try:
        socks = dict(poller.poll())
    except KeyboardInterrupt:
        break

    if socks.get(receiver) == zmq.POLLIN:
        message = receiver.recv()
        logger.info("Received message from network")
        Process(message, receiver)
