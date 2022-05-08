"""
TODO: move to common place for access from tests and scp?
"""
import struct

class CallServerEvents:
    GCEV_OFFERED = 0
    GCEV_ANSWERED = 1
    GCEV_CONNECTED = 2
    GCEV_DISCONNECTED = 3
    GCEV_DROPCALL = 4

"""
class CallServerCommands:
    CMD_DROP = 0
    CMD_PLAY = 1
"""
class StreamReadException(Exception): pass
class EvOffered:
    def __init__(self):
        self.CgPN=''
        self.CdPN=''
        self.RdPN=''
        self.reason=0

    def unpack(self, data):
        try:
            paramPtr = 0
            self.CgPN, paramPtr = self.readStr(data, paramPtr)
            self.CdPN, paramPtr = self.readStr(data, paramPtr)
            self.RdPN, paramPtr = self.readStr(data, paramPtr)
            self.reason, paramPtr = self.read2Bytes(data, paramPtr)
        except StreamReadException:
            return False

        return True

    """
    Read param len from buffer starting at data[paramPtr], then read len chars from buffer.
    Return string that was read and next paramPtr.
    Exception if buffer does not have enough data
    """
    @staticmethod
    def readStr(data, paramPtr):
        if paramPtr >= len(data): raise StreamReadException()
        paramLen = data[paramPtr]
        if len(data) < paramPtr+1+paramLen: raise StreamReadException()
        return (data[paramPtr+1:paramPtr+1+paramLen].decode(), paramPtr+1+paramLen)

    @staticmethod
    def read2Bytes(data, paramPtr):
        if len(data) < paramPtr + 2: raise StreamReadException()
        return (struct.unpack('H', data[paramPtr:paramPtr+2])[0], paramPtr + 3)

    @staticmethod
    def pack(CgPN, CdPN, RdPN, reason):
        data = bytearray()
        data += struct.pack('B', len(CgPN))
        data += bytearray(CgPN, 'utf-8')
        data += struct.pack('B', len(CdPN))
        data += bytearray(CdPN, 'utf-8')
        data += struct.pack('B', len(RdPN))
        data += bytearray(RdPN, 'utf-8')
        data += struct.pack('H', reason)
        return data

    def __repr__(self):
        S = 'CgPN : %s   CdPN : %s    RdPN : %s    reason : %d' % (self.CgPN, self.CdPN, self.RdPN, self.reason)
        return S

class CallServerCommand:

    CMD_DROP = 0
    CMD_ANSWER = 1
    CMD_PLAY = 2

    @staticmethod
    def dropCall(reason):
        data = bytearray()
        data += struct.pack('B', CallServerCommand.CMD_DROP)
        data += struct.pack('H', reason)
        return data

    @staticmethod
    def answerCall():
        data = bytearray()
        data += struct.pack('B', CallServerCommand.CMD_ANSWER)
        return data

    @staticmethod
    def playFragment(fragment):
        data = bytearray()
        data += struct.pack('B', CallServerCommand.CMD_PLAY)
        data += struct.pack('H', len(fragment))
        data += fragment.encode()
        return data
    

# Unittest
if __name__=="__main__":
    # Offered
    CgPN = '+79161111111'
    CdPN = '+79162222222'
    RdPN = '+79163333333'
    reason = 4096
    evOffered = EvOffered()
    payload = EvOffered.pack(CgPN, CdPN, RdPN, reason)
    print("Encoded:")
    print(payload)
    evOffered.unpack(payload)
    print("Decoded:")
    print(evOffered)
    assert(CgPN == evOffered.CgPN)
    assert(CdPN == evOffered.CdPN)
    assert(RdPN == evOffered.RdPN)
    assert(reason == evOffered.reason)

    # Play
    print("\nTest2: Play")
    fragmentName = "test_fragment.wav"
    serialized = CallServerCommand.playFragment(fragmentName)
    print(serialized)
    assert(serialized[0] == CallServerCommand.CMD_PLAY)
    assert(serialized[1] + serialized[2] * 256 == len(fragmentName))
    assert(serialized[3:] == fragmentName.encode())
    