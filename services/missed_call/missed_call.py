import sys
sys.path.append('..')
from scp import SCP
from logger import getLogger
#from services.sdp import SDP
from sdp_test import SDP

logger = getLogger("MissedCall")

class MissedCall(SCP):

    def __init__(self, config, dbconfig):
        SCP.__init__(self, config)
        self.config = config
        self.serviceKey = config.serviceKey
        self.needOnlyOne = config.needOnlyOne
        self.sdp = SDP(dbconfig.username, dbconfig.password, dbconfig.database)

    def onOffered(self, event):
        cgPN = event.CgPN
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



if __name__ == "__main__":
    import config
    import dbconfig
    logger.info("Starting...")
    service = MissedCall(config, dbconfig)
    service.serve_forever()
