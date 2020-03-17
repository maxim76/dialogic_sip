"""
Module provides formatted output for project modules
It creates root logger with name specified in config file, set formatting and exposes getLogger() function

Usage in modules:
-----------------
from logger import getLogger
logger = getLogger(<module name>)   # Module name should be empty for main (root) module
logger.info(...)
"""
import logging
import logging.handlers
import datetime
import config
import os

logger = logging.getLogger(config.rootLoggerName)
logger.setLevel(logging.DEBUG)

# Configure console log handler
consoleHandler = logging.StreamHandler()
consoleHandler.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s %(levelname)-8s %(name)-24s %(message)s', datefmt='%Y/%m/%d %H:%M:%S')
consoleHandler.setFormatter(formatter)
logger.addHandler(consoleHandler)

# Configure file log handler
if not os.path.exists(config.logDir):
    os.makedirs(config.logDir)
logFileName = datetime.datetime.now().strftime('%Y%m%d_%H%M%S.log')
fileHandler = logging.FileHandler(config.logDir + '/' + logFileName)
fileHandler.setLevel('DEBUG')
fileHandler.setFormatter(formatter)
logger.addHandler(fileHandler)

#Configure socket handler
#socketHandler = logging.handlers.SocketHandler(config.logServerHostname, logging.handlers.DEFAULT_TCP_LOGGING_PORT)
#logger.addHandler(socketHandler)

def getLogger(moduleName = ""):
    """
    Returns logger for the specified module, that are in the project's logger hierarchy

    @param moduleName   :   name, under which the module logs should be visible in console. If empty, then root name (from config file) will be used.
                            Otherwise, the name will be attached to namespace hierarchy under root name.
    """
    if moduleName and moduleName[0] != '.':
        moduleName = '.' + moduleName
    return logging.getLogger(config.rootLoggerName + moduleName)
