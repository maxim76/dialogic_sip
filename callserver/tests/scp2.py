import logging
import sys
import time
import zmq

import scp_interface

# Configure logger
logger = logging.getLogger('SCP')
logger.setLevel(logging.DEBUG)
consoleHandler = logging.StreamHandler()
formatter = logging.Formatter('%(asctime)s %(levelname)-8s %(name)-24s %(message)s', datefmt='%Y/%m/%d %H:%M:%S')
consoleHandler.setFormatter(formatter)
logger.addHandler(consoleHandler)


def Process(data):
    logger.info("onRequestReceived :")

    if len(data) == 0:
        logger.error("Malformed request")
        return

    # get event type and call corresponding handler
    if data[0] == scp_interface.CallServerEvents.GCEV_OFFERED:
        event = scp_interface.EvOffered()
        if not event.unpack(data[1:]):
            logger.error("onRequestReceived : Malformed request")
            return
        else:
            logger.debug("onRequestReceived : GCEV_OFFERED")
            logger.debug(event)
            # print params
    else:
        logger.error("onRequestReceived : Unhandled event code %d" % data[0])



context = zmq.Context()

# Socket to receive messages on
receiver = context.socket(zmq.PULL)
#receiver.connect("tcp://localhost:5557")
receiver.bind("tcp://*:5558")

# Socket to send messages to
sender = context.socket(zmq.PUSH)
#sender.connect("tcp://localhost:5558")
sender.bind("tcp://*:5557")

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
        Process(message)
