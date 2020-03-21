from logger import getLogger
from userfuncs import uf2015, uf2063, uf3031, uf3014

logger = getLogger("SCP")

class SDP:
    def __init__(self, username, password, database):
        self.uf3031 = uf3031.UF3031(username, password, database)
        self.uf2015 = uf2015.UF2015(username, password, database)
        self.uf2063 = uf2063.UF2063(username, password, database)
        self.uf3014 = uf3014.UF3014(username, password, database)

    def checkService(self, addrSrc, serviceKey):
        logger.debug("SDP::checkService()")
        result = self.uf3031.execute({'io_AddrSrc_value':addrSrc, 'i_ServiceKey':serviceKey})
        logger.debug("SDP::checkService() : result %s" % str(result))
        if result['o_RetVal'].getvalue() == 1:
            outparams = dict()
            outparams['ServiceSN'] = int(result['o_ServiceSN'].getvalue())
            outparams['NotifType'] = int(result['o_NotifType'].getvalue())
            return (1, outparams)
        else:
            logger.error("SDP::checkService() : Error type : %d Error data %s" % (int(result['o_ContentError$type'].getvalue()), result['o_ContentError$data'].getvalue()))
            return (0, None)

    def addToHistory(self, serviceSN, aNum, abonType, reason, needOnlyOne):
        logger.debug("SDP::addToHistory()")
        result = self.uf2015.execute({
            'i_ServiceSN':serviceSN,
            'i_ANum':aNum,
            'i_AbonType':abonType,
            'i_Reason':reason,
            'i_NeedOnlyOne':needOnlyOne
        })
        logger.debug("SDP::addToHistory() : result %s" % str(result))
        if result['o_RetVal'].getvalue() != 0:
            outparams = dict()
            outparams['HistorySN'] = int(result['o_RetVal'].getvalue())
            outparams['SMSInfo'] = result['o_SMSInfo$data'].getvalue()
            return (1, outparams)
        else:
            logger.error("SDP::addToHistory() : Error type : %d Error data %s" % (int(result['o_ContentError$type'].getvalue()), result['o_ContentError$data'].getvalue()))
            return (0, None)

    def getNotifAddresses(self, serviceSN, notifType):
        logger.debug("SDP::getNotifAddresses()")
        result = self.uf2063.execute({
            'i_ServiceSN':serviceSN,
            'i_NotifType':notifType
        })
        logger.debug("SDP::getNotifAddresses() : result %s" % str(result))
        if result is not None:
            return (1, result)
        else:
            logger.error("SDP::getNotifAddresses() : Empty result")
            return (0, None)
        

    def saveSMSHistory(self, historySN, SMSID):
        logger.debug("SDP::saveSMSHistory()")
        result = self.uf3014.execute({
            'i_HistorySN':historySN,
            'i_SMSList':SMSID
        })
        logger.debug("SDP::saveSMSHistory() : result %s" % str(result))
        if result is not None:
            if result['o_RetVal'].getvalue() != 0:
                return (1, result)
            else:
                return (0, None)
        else:
            logger.error("SDP::saveSMSHistory() : Empty result")
            return (0, None)

# Unittest
if __name__=="__main__":
    import config
    import dbconfig
    sdp = SDP(dbconfig.username, dbconfig.password, dbconfig.database)
    result = sdp.checkService(config.unittestSDPAddrSrc, config.unittestSDPServiceKey)
    print(result)