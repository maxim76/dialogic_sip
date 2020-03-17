"""
This module implements integration with Message Queue.
RabbitMQ (AMQP) is supported at the moment.
"""
import pika

from logger import getLogger
from plugin import Plugin
from stat_counters import StatCounters


class MQClient(Plugin):
    """
    Base class for plugins that interacts with Message Queue. Class implements integration with RabbitMQ using AMQP.
    """
    def __init__(self, mqServer, mqUsername, mqPassword, queueName = "", isDurable = True):
        """
        Class constructor establishes connection with rabbitmq-server

        @param mqServer     : hostname of server with rabbitmq-server instance
        @param mqUsername   : username that will be used for opening connection to message queue
        @param mqPassword   : password that will be used for opening connection to message queue
        @param queueName    : queue name, where request shoud be read from
        @param isDurable    : flag that indicates if the queue is persistent in rabbitmq-server
        """
        self.logger = getLogger("MQClient")
        credentials = pika.PlainCredentials(mqUsername, mqPassword)
        self.connection = pika.BlockingConnection(pika.ConnectionParameters(mqServer, credentials = credentials))
        result = self.connection.channel().queue_declare(queue=queueName, durable=isDurable)
        # If queue name was not provided in parameter, it will be autogenerated
        self.queueName = result.method.queue
        self.logger.info("Connected to '%s'" % self.queueName)

    def update(self):
        """
        Processes queue tasks: checks if queue has new messages and calls the callback, serves keepalive etc. Must be called periodically
        """
        self.connection.process_data_events()
        '''
        except Exception as e:
            self.logger.critical("Exception when tried to interact with MQ: %s" % str(e))
            import sys
            sys.exit()
        '''

    def __del__(self):
        """
        Class destructor. Closes connection with MQ-server
        """
        self.connection.close()


class MQListener(MQClient):
    """
    Class implements message receiving from a Message Queue
    """

    class AckInfo:
        """
        This class encapsulates information required for acknowledging a message
        """
        def __init__(self, channel, tag):
            """
            @param channel  : pika.channel that received message
            @param tag      : message's delivery tag
            """
            self.channel = channel
            self.tag = tag


    def __init__(self, mqServer, mqUsername, mqPassword, queueName = "", userCallback = None, isDurable = True):
        """
        Class constructor establishes connection with rabbitmq-server and sets the queue for messages consumption

        @param mqServer     : hostname of server with rabbitmq-server instance
        @param queueName    : queue name, where request shoud be read from
        @param userCallback : callback function that processes requests
        @param isDurable    : flag that indicates if the queue is persistent in rabbitmq-server
        """
        self.logger = getLogger("MQListener")
        MQClient.__init__(self, mqServer, mqUsername, mqPassword, queueName, isDurable)

        self.connection.channel().basic_qos(prefetch_count=1)
        self.terminated = False
        self.userCallback = userCallback
        self.tag = None

        self.connection.channel().basic_consume(
            queue=queueName,
            auto_ack=False,
            on_message_callback=self.callback)

    def callback(self, channel, method, properties, body):
        """
        Callback that is invoked when new message is read from the queue

        @param channel      : pika.Channel
        @param method       : pika.spec.Basic.Deliver
        @param properties   : pika.spec.BasicProperties
        @param body         : bytes received
        """
        self.logger.debug("Recv: [%s]. Delivery tag = %d" % (str(body), method.delivery_tag))
        StatCounters.Requests.received += 1
        self.tag = method.delivery_tag
        if self.userCallback:
            self.userCallback(body, self.AckInfo(channel, method.delivery_tag))
        else:
            self.logger.warning("Callback is not configured. Request is ignored")

    def ack(self, ackInfo):
        """
        Sends acknowledgement to RabbitMQ, confirming that message been processed

        @param ackInfo  : struct with message details required for acknowledging
        """
        self.logger.debug("Acknowledging message. Tag %d" % ackInfo.tag)
        StatCounters.Requests.processed += 1
        ackInfo.channel.basic_ack(ackInfo.tag)

    def requeue(self, ackInfo):
        """
        Sends requeue request to RabbitMQ, informing that message was not processed but should be reattempted

        @param ackInfo  : struct with message details required for acknowledging
        """
        self.logger.debug("Requeueing message. Tag %d" % ackInfo.tag)
        StatCounters.Requests.requeued += 1
        ackInfo.channel.basic_nack(ackInfo.tag, True)

    def delete(self, ackInfo):
        """
        Sends reject request to RabbitMQ, informing that message was not processed and should be deleted

        @param ackInfo  : struct with message details required for acknowledging
        """
        self.logger.debug("Deleting message. Tag %d" % ackInfo.tag)
        StatCounters.Requests.rejected += 1
        ackInfo.channel.basic_reject(ackInfo.tag, False)


class MQSender(MQClient):
    """
    Class implements message sending to a Message Queue
    """

    def __init__(self, mqServer, mqUsername, mqPassword, queueName = "", isDurable = True):
        """
        Class constructor establishes connection with rabbitmq-server and sets the queue for messages sending

        @param mqServer     : hostname of server with rabbitmq-server instance
        @param queueName    : queue name, where request shoud be sent to
        @param isDurable    : flag that indicates if the queue is persistent in rabbitmq-server
        """
        self.logger = getLogger("MQSender")
        MQClient.__init__(self, mqServer, mqUsername, mqPassword, queueName, isDurable)

    def send(self, message):
        """
        Send message to the queue

        @param message  : message to send
        """
        self.logger.debug("Send: [%s]" % str(message))
        self.connection.channel().basic_publish(
            exchange='',
            routing_key=self.queueName,
            body=message,
            properties=pika.BasicProperties(
                delivery_mode=2,  # make message persistent
            ))


# Unit-test
if __name__ == "__main__":
    import config

    logger = getLogger("MQ.Unittest")
    logger.info("%s is started in unittest mode" % __file__.split('/')[-1])

    # Send test message
    unittestQueueName = 'unittest'
    unittestMessage = b'Unittest message'
    unittestTimeout = 1
    MQSender(config.mqServer, config.mqUsername, config.mqPassword, unittestQueueName, isDurable = False).send(unittestMessage)
    logger.info("Sent message [%s]" % unittestMessage)

    # Create listener instance and try to read the message withing testing timeout
    receivedMessage = ''

    def MQCallback(messageBody, ackInfo):
        logger.info("Recv message [%s]" % messageBody)
        mqListener.terminated = True
        global receivedMessage
        receivedMessage = messageBody
        mqListener.ack(ackInfo)

    mqListener = MQListener(config.mqServer, config.mqUsername, config.mqPassword, unittestQueueName, MQCallback, isDurable = False)

    import time
    timeout = time.time() + unittestTimeout
    while (not mqListener.terminated) and (time.time() < timeout):
        mqListener.update()

    # Test that received message is equal to what has been sent
    assert(unittestMessage == receivedMessage)
    logger.info("Test passed")
