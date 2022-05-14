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
    logger.debug("onRequestReceived : [" + ''.join( ["%02X" % ch for ch in data] ) + "]")
    if len(data) <= 8:
        logger.error("onRequestReceived : Malformed request")
        return

    requestID = struct.unpack('I', data[0:4])[0]
    sessionID = struct.unpack('I', data[4:8])[0]
    command = data[8]
    logger.info("onRequestReceived : requestID [%d]  sessionID [%d] command code [%d]" % (requestID, sessionID, command))
    
    # get event type and call corresponding handler
    if command == scp_interface.CallServerEvents.GCEV_OFFERED:
        event = scp_interface.EvOffered()
        if not event.unpack(data[9:]):
            logger.error("onRequestReceived : GCEV_OFFERED Malformed request")
            return
        else:
            logger.info("onRequestReceived : GCEV_OFFERED")
            logger.debug(event)

            # Answer the call
            response = scp_interface.CallServerCommand.answerCall()
            logger.info("onOffered : sending command ANSWER")
            receiver.send(response)

    elif command == scp_interface.CallServerEvents.GCEV_ANSWERED:
        # Play file
        response = scp_interface.CallServerCommand.playFragment("ussr.wav")
        logger.info("onOffered : sending command PLAY")
        receiver.send(response)

    elif command == scp_interface.CallServerEvents.PLAY_FINISHED:
        # Drop the call
        response = scp_interface.CallServerCommand.dropCall(1234)
        logger.info("onOffered : sending command DROP with reason %d" % 1234)
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

logger.info("SCP Started")
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
