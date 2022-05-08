#pragma once

#include <sstream>
#include <string>

/*
TODO:
1. Вынести реализация в cpp
2. Базовый класс для SCP и SSP сообщений с виртуальными pack/unpack
3. Юниттесты
*/

namespace ssp_scp
{
	typedef unsigned short RedirectionReason;
#pragma pack(1)

enum SSPEventCodes
{
	OFFERED = 0,
	ANSWERED = 1
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
	bool pack( char *buffer, size_t bufferSize, size_t *filledSize ) const
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
	CMD_DROP = 0,
	CMD_ANSWER = 1,
	CMD_PLAY = 2
};


/*
* TODO: Refactor
* 1. SCPCommand вызывает свой невиртуальный unpack, который декодирует код команды а затем вызывает полиморфный Deserialize
* 2. возможно стоит сделать конструктор, который принимает стрим и создает уже десериализированный экземпляр? Или это статик метод - фабрика классов
*/
struct SCPCommand
{
	unsigned char commandCode;
	virtual bool unpack(std::istream& in) {};
};

struct CmdDrop
{
	SCPCommand scpCommand;
	RedirectionReason reason;
};

struct CmdPlay : SCPCommand
{
	std::string fragmentName;
	bool unpack(std::istream& in) override {
		uint16_t size;
		std::string str;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		fragmentName.resize(size);
		in.read(&(fragmentName[0]), size);
	}
};


};