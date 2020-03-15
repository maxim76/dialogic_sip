namespace ssp_scp
{
enum SSPEventCodes
{
	OFFERED = 0,
	ANSWERED
};

struct SSPEvent
{
	char eventCode;
};

struct Offered
{
	SSPEvent sspEvent;
	char CgPN[MAX_NUMSIZE];
	char CdPN[MAX_NUMSIZE];
	char RdPN[MAX_NUMSIZE];
	unsigned int redirectionReason;
};

};