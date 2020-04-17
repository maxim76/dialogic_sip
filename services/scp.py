import datetime
import time
import scp_interface
from logger import getLogger
from plugin_manager import PluginManager
from scp_interface import EvOffered
from udp_server import UDPServer

logger = getLogger("SCP")

class SCP:
    # TODO: decide if session is required in SCP
    '''
    class Session:
        """
        Struct to keep parameters specific to a request processing session
        """

        def __init__(self):
            """
            Class constructor creates request processing session
            """
            uniqueIDTemplate = r'%Y%m%d_%H%M%S_%f'
            self.sessionID = datetime.datetime.now().strftime(uniqueIDTemplate)  # Generate unique file name ID fo rthis session
            logger.debug("Session %s is created" % self.sessionID)

        def __del__(self):
            """
            Session destructor.
            """
            logger.debug("Session %s is destroying" % self.sessionID)
    '''

    def __init__(self, config):
        self.terminated = False     # Flag used to stop the service
        self.config = config

        # Create plugin instances and add them to manager so that they get control periodically
        # Note: plugins should be derived from class Plugin and implement update() function
        self.pluginManager = PluginManager()
        self.udpServer = UDPServer(config.scpHost, config.scpPort, self.onRequestReceived)
        self.pluginManager.add(self.udpServer)

        #self.sessions = dict()


    def onRequestReceived(self, handler, channel, data):
        """
        Callback that is invoked when new request come

        @param handler  :   instance of SCPRequestHandler, that invoked this callback
        @param channel  :   IVR channel number where event happened
        @data           :   raw content of the request
        """
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
                # TODO: Create session here?
                '''
                session = self.Session()
                self.sessions.add(session)
                '''
                self.onOffered(channel, event, handler)
        else:
            logger.error("onRequestReceived : Unhandled event code %d" % data[0])
            handler.sendResponse(channel, data)


    def onOffered(self, channel, event, handler):
        logger.debug("SCP::onOffered() : implement me")
        
    def serve_forever(self):
        """
        Main service loop that listen and process requests
        """
        logger.info("Enter the main loop")
        try:
            while (not self.terminated):
                self.pluginManager.processPlugins()
                time.sleep(self.config.tick)
        except KeyboardInterrupt:
            logger.warning("KeyboardInterrupt")

        #logger.info("Stopping background activities...")
        #TODO: stop treads


if __name__ == "__main__":
    import config
    logger.info("Starting...")
    scp = SCP(config)
    scp.serve_forever()
