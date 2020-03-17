import struct

class CallServerEvents:
    GCEV_OFFERED = 0
    GCEV_ANSWERED = 1
    GCEV_CONNECTED = 2
    GCEV_DISCONNECTED = 3
    GCEV_DROPCALL = 4


class EvOffered:
    def __init__(self):
        self.CgPN=''
        self.CdPN=''
        self.RdPN=''
        self.reason=0

    def unpack(self, data):
        # read CgPN
        paramPtr = 0
        paramLen = data[paramPtr]
        self.CgPN = data[paramPtr+1:paramPtr+1+paramLen].decode()
        # read CdPN
        paramPtr += paramLen + 1
        paramLen = data[paramPtr]
        self.CdPN = data[paramPtr+1:paramPtr+1+paramLen].decode()
        # read RdPN
        paramPtr += paramLen + 1
        paramLen = data[paramPtr]

        self.RdPN = data[paramPtr+1:paramPtr+1+paramLen].decode()
        # read reason
        paramPtr += paramLen + 1
        self.reason = struct.unpack('H', data[paramPtr:paramPtr+2])[0]

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


# Unittest
if __name__=="__main__":
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
