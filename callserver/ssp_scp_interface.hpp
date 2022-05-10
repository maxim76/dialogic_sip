#pragma once

#include <sstream>
#include <cstring>	// strlen
#include <string>

/*
TODO:
1. Вынести реализация в cpp
2. Базовый класс для SCP и SSP сообщений с виртуальными pack/unpack
3. Юниттесты
4. Заменить типы данных CxPN на стринги и избавится от define MAX_NUMSIZE
*/
#define SSP_SCP_MAX_NUMSIZE	128		// Временно. Должно быть эквивалентно MAX_NUMSIZE из callserver.hpp

namespace ssp_scp
{
// Классы реализующие этот интерфейс могут сериализоваться в поток (используется для передачи по сети)
class ISerializable {
public:
	virtual void serialize(std::ostream& out) const = 0;
};

typedef unsigned short RedirectionReason;
#pragma pack(1)

enum SSPEventCodes
{
	OFFERED = 0,
	ANSWERED,
	CONNECTED,
	DISCONNECTED,
	DROPCALL,
	PLAY_FINISHED
};

class SSPEvent
{
public:
	SSPEvent(unsigned char eventCode) : eventCode_(eventCode) {}

	const unsigned char eventCode_;
};

struct SSPEvent2 {
	unsigned char eventCode;
};

static_assert(SSP_SCP_MAX_NUMSIZE < 256, "String field size must fit 1 byte");
struct Offered
{
	SSPEvent2 sspEvent;
	char CgPN[SSP_SCP_MAX_NUMSIZE];
	char CdPN[SSP_SCP_MAX_NUMSIZE];
	char RdPN[SSP_SCP_MAX_NUMSIZE];
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


class SSPEventOnPlayFinished : public SSPEvent, public ISerializable {
public:
	SSPEventOnPlayFinished(int result) : 
		SSPEvent(SSPEventCodes::PLAY_FINISHED),
		result_(result)
	{}

	void serialize(std::ostream& out) const override {
		out.write(reinterpret_cast<const char*>(&eventCode_), sizeof(eventCode_));
		out.write(reinterpret_cast<const char*>(&result_), sizeof(result_));
	}

private:
	int result_;
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