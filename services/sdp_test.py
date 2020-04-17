"""
Test module to substiture real SDP. Do not interact with DB, just imitate time-consuming task and return predefined result
"""
from logger import getLogger
import time

logger = getLogger("SDPTest")

class SDP:
    DEALY_CHECKSERVICE = 0.3
    DELAY_ADDTOHISTORY = 0.2
    DELAY_GETNOTIFADDRESS = 0.3
    DELAY_SAVESMSHISTORY = 0.2

    RESULT_SERVICESN = 11111
    RESULT_NOTIFTYPE = 2

    RESULT_HISTORYSN = 22222
    RESULT_SMSINFO = 'SMS Template from Test SDP'

    RESULT_ADDRSN = 33333
    RESULT_NUMVALUE = '+79160000000'
    RESULT_NUMTYPE = 256

    RESULT_RETVAL = 44444

    def __init__(self, username, password, database):
        pass

    def checkService(self, addrSrc, serviceKey):
        time.sleep(self.DEALY_CHECKSERVICE)
        outparams = dict()
        outparams['ServiceSN'] = self.RESULT_SERVICESN
        outparams['NotifType'] = self.RESULT_NOTIFTYPE
        logger.debug("SDP::checkService() : result %s" % str(outparams))
        return (1, outparams)

    def addToHistory(self, serviceSN, aNum, abonType, reason, needOnlyOne):
        time.sleep(self.DELAY_ADDTOHISTORY)
        outparams = dict()
        outparams['HistorySN'] = self.RESULT_HISTORYSN
        outparams['SMSInfo'] = self.RESULT_SMSINFO
        logger.debug("SDP::addToHistory() : result %s" % str(outparams))
        return (1, outparams)

    def getNotifAddresses(self, serviceSN, notifType):
        time.sleep(self.DELAY_GETNOTIFADDRESS)
        outparams = dict()
        outparams['AddrSN'] = self.RESULT_ADDRSN
        outparams['Num_value'] = self.RESULT_NUMVALUE
        outparams['Num_type'] = self.RESULT_NUMTYPE
        logger.debug("SDP::getNotifAddresses() : result %s" % str(outparams))
        return (1, outparams)
        

    def saveSMSHistory(self, historySN, SMSID):
        time.sleep(self.DELAY_SAVESMSHISTORY)
        outparams = dict()
        outparams['o_RetVal'] = self.RESULT_RETVAL
        logger.debug("SDP::saveSMSHistory() : result %s" % str(outparams))
        return (1, outparams)


# Unittest
if __name__=="__main__":
    import config
    import dbconfig
    sdp = SDP(dbconfig.username, dbconfig.password, dbconfig.database)
    result = sdp.checkService(config.unittestSDPAddrSrc, config.unittestSDPServiceKey)
    logger.info(result)