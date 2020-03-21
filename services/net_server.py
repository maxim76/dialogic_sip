import socketserver

MAX_BUF_SIZE = 4096


class NetServer:
	"""
	TCP/UDP server framework (milti-thread). Provides send to network methods and receive callbacks
	"""

	class TCPUDPHandler(socketserver.BaseRequestHandler):
		"""
		Internal callback class that invokes user's callback function
		"""
		def handle( self ):
			"""
			Internal TCP/UDP on-data-receive callback funcation
			"""
			if self.proto == 'UDP':
				data = self.request[0].strip()
				self.socket = self.request[1]
			elif self.proto == 'TCP':
				data = self.request.recv( MAX_BUF_SIZE ).strip()
			else:
				raise 'Unknown protocol "%s"' % self.proto
			self.externalHandler( self, self.client_address, data )
		# handle

		def send( self, data ):
			"""
			Sends data to client socket

			@param data	: data to send (as python string)
			"""
			if self.proto == 'UDP':
				self.socket.sendto( data, self.client_address )
			else:
				self.request.sendall( data )
		# send

	# class TCPUDPHandler

	def __init__( self, proto, host, port ):
		"""
		Initialises server

		@param proto	: protocol. Acceptable values 'TCP' or 'UDP'
		@param host		: hostname or IP-address of local socket
		@param port		: local port
		"""

		self.proto = proto
		self.host = host
		self.port = port

		if not proto in ['UDP', 'TCP']:
			raise 'Unknown protocol "%s"' % proto

		self.TCPUDPHandler.proto = proto
		self.TCPUDPHandler.externalHandler = self.onReceive

		if proto == 'UDP':
			self.server = socketserver.ThreadingUDPServer( (host, port), self.TCPUDPHandler )
		else:
			self.server = socketserver.ThreadingTCPServer( (host, port), self.TCPUDPHandler )
	#  __init__

	def onReceive( self, requestHandler, clientAddress, data ):
		"""
		User's on-data-receive callback. Should be overriden in subclass

		@param requestHandler	: instance of TCPUDPHandler that received the requiest
		@param clientAddress	: remote (host, port)
		@param data				: data to send
		"""
		print( "NetServer:onReceive() : override me" )
		print( "    ", "%s port %d got data from %s:" % ( self.proto, self.port, str( clientAddress ) ) )
		print( "     Dump: ", ''.join( ["0x%02x " % ord(ch) for ch in data] ) )
	# handler

	def run( self ):
		"""
		Handle requests until an explicit server.shutdown() request
		"""
		self.server.serve_forever()
	# run

	def done( self ):
		"""
		Clean up the server
		"""
		self.server.server_close()
	# done

# class NetServer


# Unit-test
if __name__ == "__main__":
	testPort = 8080
	netServer = NetServer( "UDP", '', testPort )
	import thread
	import time
	from Interfaces.net import UDP

	print( "Starting server in new thread" )
	thread.start_new_thread( netServer.run, () )
	print( "Sending UDP packet to server" )
	UDP().send( '127.0.0.1', testPort, bytearray.fromhex( '01020304' ) )
	time.sleep( 1 )
	print( "Stopping server" )
	netServer.server.shutdown()
	netServer.done()
