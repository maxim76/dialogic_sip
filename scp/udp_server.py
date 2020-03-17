import socketserver
from functools import partial
from logger import getLogger
from plugin import Plugin

logger = getLogger("UDPServer")

class UDPServer(Plugin):

    class SCPRequestHandler(socketserver.BaseRequestHandler):

        def __init__(self, callback):
            self.callback = callback

        def handle(self):
            data = self.request[0]
            socket = self.request[1]
            print( "SCPRequestHandler::handle() : Recv [" + ''.join( ["%02X " % ch for ch in data] ) + "]" )
            self.callback()

    def __init__(self, host, port, callback):
        #self.callback = callback
        handler = partial(self.SCPRequestHandler, callback)
        #self.server = socketserver.UDPServer((host, port), self.SCPRequestHandler)
        self.server = socketserver.UDPServer((host, port), handler)

    def update(self):
        self.server.handle_request()

class SCPRequestHandler(socketserver.BaseRequestHandler):

    def handle(self):
        data = self.request[0]
        socket = self.request[1]
        print( "SCPRequestHandler::handle() : Recv [" + ''.join( ["%02X " % ch for ch in data] ) + "]" )
        if len(data)<5:
            print("Malformed request")
            return

        #socket.sendto(data.upper(), self.client_address)


# Unittest
if __name__ == "__main__":
    unittestHost = '192.168.1.107'
    unittestPort = 9999
    server_address = (unittestHost, unittestPort)
    print('Starting UDP server on %s port %s' % server_address)

    '''
    with socketserver.UDPServer(server_address, SCPRequestHandler) as server:
        server.serve_forever()
    '''
    def callback():
        print("Request received")

    udpServer = UDPServer(unittestHost, unittestPort, callback)
    import time
    while(True):
        udpServer.update()
        time.sleep(1)