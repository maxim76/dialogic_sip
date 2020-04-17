import socketserver
import struct
from functools import partial
from logger import getLogger
from plugin import Plugin

logger = getLogger("UDPServer")

class UDPServer(Plugin):

    class SCPRequestHandler(socketserver.BaseRequestHandler):

        def __init__(self, callback, *args, **kwargs):
            self.callback = callback
            self.requests = dict()
            socketserver.BaseRequestHandler.__init__(self, *args, **kwargs)

        def handle(self):
            data = self.request[0]
            socket = self.request[1]
            logger.debug( "Recv [" + ''.join( ["%02X " % ch for ch in data] ) + "]" )
            if len(data) < 5:
                logger.error("Malformed request")
                return
            channel = struct.unpack_from('I', data)[0]
            self.requests[channel] = (socket, self.client_address)
            self.callback(self, channel, data[4:])

        def sendResponse(self, channel, data):
            request = self.requests.get(channel, None)
            if request is None:
                logger.error('sendResponse : no request on channel %d' % channel)
                return

            buffer = bytearray()
            buffer += struct.pack( 'I', channel )
            buffer += data
            socket, address = request[0], request[1]
            socket.sendto(buffer, address)
            logger.debug( "Send [" + ''.join( ["%02X " % ch for ch in buffer] ) + "]" )


    def __init__(self, host, port, callback):
        handler = partial(self.SCPRequestHandler, callback)
        self.server = socketserver.UDPServer((host, port), handler)
        logger.info('UDPServer : started on %s port %s' % (host, port))

    def update(self):
        self.server.handle_request()


# Unittest
if __name__ == "__main__":

    logger = getLogger("UDPServer.Unittest")
    logger.info("%s is started in unittest mode" % __file__.split('/')[-1])
    logger.info("All incoming messages will be echoed back")

    unittestHost = '192.168.1.107'
    unittestPort = 9999
    server_address = (unittestHost, unittestPort)

    def callback(self, channel, data):
        logger.info("callback() : Request received at channel %d" % channel)
        self.sendResponse(channel, data)


    udpServer = UDPServer(unittestHost, unittestPort, callback)
    import time
    while(True):
        udpServer.update()
        time.sleep(1)