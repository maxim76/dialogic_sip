#pragma once

#include <sstream>
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

class SSPEvent : public ISerializable
{
public:
	SSPEvent(unsigned char eventCode) : eventCode_(eventCode) {}

	void serialize(std::ostream& out) const override {
		// Write Event code
		out.write(reinterpret_cast<const char*>(&eventCode_), sizeof(eventCode_));
		// Write Event specific data
		serializeImpl(out);
	}

	virtual void serializeImpl(std::ostream& out) const = 0;

private:
	const unsigned char eventCode_;
};

class SSPEventOffered : public SSPEvent {
public:
	SSPEventOffered(std::string CgPN, std::string CdPN, std::string RdPN, RedirectionReason reason) :
		SSPEvent(SSPEventCodes::OFFERED),
		CgPN_(std::move(CgPN)),
		CdPN_(std::move(CdPN)),
		RdPN_(std::move(RdPN)),
		reason_(reason)
	{}

	void serializeImpl(std::ostream& out) const override {
		// CgPN
		size_t size = CgPN_.size();
		out.write(reinterpret_cast<const char*>(&size), sizeof(char));
		out.write(CgPN_.data(), size);
		// CdPN
		size = CdPN_.size();
		out.write(reinterpret_cast<const char*>(&size), sizeof(char));
		out.write(CdPN_.data(), size);
		// RdPN
		size = RdPN_.size();
		out.write(reinterpret_cast<const char*>(&size), sizeof(char));
		out.write(RdPN_.data(), size);
		// redirectionReason
		out.write(reinterpret_cast<const char*>(&reason_), sizeof(reason_));
	}

private:
	std::string CgPN_;
	std::string CdPN_;
	std::string RdPN_;
	RedirectionReason reason_;
};

class SSPEventAnswered : public SSPEvent {
public:
	SSPEventAnswered() :
		SSPEvent(SSPEventCodes::ANSWERED)
	{}

	void serializeImpl(std::ostream& out) const override {
		// Nothing to write, ANSWERED does not have parameters
	}
};

class SSPEventOnPlayFinished : public SSPEvent {
public:
	SSPEventOnPlayFinished(int result) : 
		SSPEvent(SSPEventCodes::PLAY_FINISHED),
		result_(result)
	{}

	void serializeImpl(std::ostream& out) const override {
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