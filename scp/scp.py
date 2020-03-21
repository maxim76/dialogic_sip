import time
import scp_interface
from logger import getLogger
from message_queue import MQListener, MQSender
from plugin import Plugin
#from sdp import SDP
from sdp_test import SDP
from scp_interface import EvOffered
from udp_server import UDPServer

logger = getLogger("SCP")

class SCP:
    class PluginManager:
        """
        Class keeps a list of separate plugins and gives them CPU time for executing
        """
        def __init__(self):
            """
            Constructor, initiate empty plugin list
            """
            self.plugins = []

        def add(self, plugin):
            """
            Adds plugin to the list of plugins that are executed
            """
            assert isinstance(plugin, Plugin)
            self.plugins.append(plugin)

        def processPlugins(self):
            """
            Gives CPU time to plugins to execute their part of work.
            Work that are being done in every plugin by single call to plugin's update() should be short enough in order not to block the main thread
            """
            for plugin in self.plugins:
                plugin.update()

    def __init__(self, config, dbconfig):
        self.config = config
        self.serviceKey = config.serviceKey
        self.needOnlyOne = config.needOnlyOne
        self.sdp = SDP(dbconfig.username, dbconfig.password, dbconfig.database)
        self.terminated = False     # Flag used to stop the service

        # Create plugin instances and add them to manager so that they get control periodically
        # Note: plugins should be derived from class Plugin and implement update() function
        self.pluginManager = self.PluginManager()
        self.udpServer = UDPServer(config.scpHost, config.scpPort, self.onRequestReceived)
        self.pluginManager.add(self.udpServer)


    def onRequestReceived(self, handler, channel, data):
        """
        Callback that is invoked when new request come

        @param handler  :   instance of SCPRequestHandler, that invoked this callback
        @param channel  :   IVR channel number where event happened
        @data           :   raw content of the request
        """
        logger.info("onRequestReceived() :")
        handler.sendResponse(channel, data)

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
                self.onOffered(event)



    '''
    def onRequestReceived(self, payload, ackInfo):
        """
        Callback that is invoked when new request come

        @param payload  :   raw content of the request
        @ackInfo        :   struct with MQ details required for acknowledging the request
        """
        logger.info("onRequestReceived() :")
        evOffered = EvOffered()
        evOffered.unpack(payload)
        logger.info("onRequestReceived() : [%s]" % evOffered)

        self.onOffered(evOffered.CgPN, evOffered.RdPN, evOffered.reason)
        """
        message = json.loads(payload)
        logger.info("onRequestReceived() : Request Received. Content: %r" % message)
        downloadURL = message.get('replay_download_url', None)
        if downloadURL is None:
            logger.error("Empty replay_download_url in the request")
            StatCounters.Requests.malformed += 1
            self.mqListener.delete(ackInfo)
        else:
            # Create new session
            session = self.Session(self.config, ackInfo, message.get('job_id', ''))
            session.downloadURL = downloadURL
            # Start replay file downloading
            session.attemptCount = 0
            ActionHandler.processEvent(Events.ONREQUESTRECEIVED, session)
        """
    '''

    def onOffered(self, event):
        cgPN = event.CgPN
        cdPN = event.CdPN
        rdPN = event.RdPN
        reason = event.reason
        logger.debug("onOffered()")
        # Check service
        result = self.sdp.checkService(rdPN, self.serviceKey)
        if result[0] == 0:
            logger.error("onOffered() : checkService error")
            return
        logger.debug("onOffered() : checkService returned %s" % str(result))
        serviceSN = result[1]['ServiceSN']
        notifType = result[1]['NotifType']

        # Add to History
        result = self.sdp.addToHistory(serviceSN, cgPN, 1, reason, self.needOnlyOne)
        if result[0] == 0:
            logger.error("onOffered() : addToHistory error")
            return
        logger.debug("onOffered() : addToHistory returned %s" % str(result))
        historySN = result[1]['HistorySN']
        smsInfo = result[1]['SMSInfo']

        # Get notification address
        result = self.sdp.getNotifAddresses(serviceSN, notifType)
        if result[0] == 0:
            logger.error("onOffered() : getNotifAddresses error")
            return
        logger.debug("onOffered() : getNotifAddresses returned %s" % str(result))
        addrSN = result[1]['AddrSN']
        smsNum = result[1]['Num_value']

        # Send notification
        # TODO: SMPP SUBMIT
        smsID = "1111111"

        # Save SMS history
        result = self.sdp.saveSMSHistory(historySN, smsID)
        if result[0] == 0:
            logger.error("onOffered() : saveSMSHistory error")
            return
        logger.debug("onOffered() : saveSMSHistory returned %s" % str(result))

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
    import dbconfig
    logger.info("Starting...")
    scp = SCP(config, dbconfig)
    scp.serve_forever()
