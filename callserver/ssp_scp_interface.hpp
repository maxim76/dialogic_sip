namespace ssp_scp
{
	typedef unsigned short RedirectionReason;
#pragma pack(1)

enum SSPEventCodes
{
	OFFERED = 0,
	ANSWERED
};

struct SSPEvent
{
	unsigned char eventCode;
};
static_assert(MAX_NUMSIZE < 256, "String field size must fit 1 byte");
struct Offered
{
	SSPEvent sspEvent;
	char CgPN[MAX_NUMSIZE];
	char CdPN[MAX_NUMSIZE];
	char RdPN[MAX_NUMSIZE];
	RedirectionReason redirectionReason;
	bool pack( char *buffer, size_t bufferSize, size_t *filledSize )
	{
		size_t lenCgPN = strlen( CgPN );
		size_t lenCdPN = strlen( CdPN );
		size_t lenRdPN = strlen( RdPN );
		size_t totalSize = 1 + 1 + lenCgPN + 1 + lenCdPN + 1 + lenRdPN + sizeof(RedirectionReason);
		if(bufferSize < totalSize) return false;

		size_t pos = 0;
		// Event code
		buffer[pos] = sspEvent.eventCode;
		++pos;
		// CgPN
		buffer[pos] = lenCgPN;
		++pos;
		strncpy(&(buffer[pos]), CgPN, bufferSize - pos);
		pos += lenCgPN;
		// CdPN
		buffer[pos] = lenCdPN;
		++pos;
		strncpy( &(buffer[pos]), CdPN, bufferSize - pos );
		pos += lenCdPN;
		// RdPN
		buffer[pos] = lenRdPN;
		++pos;
		strncpy( &(buffer[pos]), RdPN, bufferSize - pos );
		pos += lenRdPN;
		// redirectionReason
		// pack 2 bytes as little-endian
		buffer[pos] = redirectionReason & (0xff);
		buffer[pos + 1] = (redirectionReason & (0xff00)) >> 8;

		*filledSize = totalSize;
		return true;
	}
};

enum SCPCommandCodes
{
	CMD_DROP = 0
};

struct SCPCommand
{
	unsigned char commandCode;
};

struct CmdDrop
{
	SCPCommand scpCommand;
	RedirectionReason reason;
};

};