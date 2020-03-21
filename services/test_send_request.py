"""
FrostClientService test tool. Sends single request
"""
import pika
import config
import json
#from job_message_models.job_message  import JobMessage
from logger import getLogger
from scp_interface import EvOffered


logger = getLogger("TestSendMessage")


connection = pika.BlockingConnection(pika.ConnectionParameters(config.mqServer))
channel = connection.channel()
channel.queue_declare(queue=config.mqRequestQueueName, durable=True)

CgPN = '+79161111111'
CdPN = '+79162222222'
RdPN = '+79163333333'
reason = 4096
payload = EvOffered.pack(CgPN, CdPN, RdPN, reason)

channel.basic_publish(  exchange='',
                        routing_key=config.mqRequestQueueName,
                        body=payload)
logger.info("Sent [%r]" % payload)
connection.close()
