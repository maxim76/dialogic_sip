#include "ssp_scp_interface.hpp"

namespace ssp_scp {

// TODO: channel передается как параметр в process(), а остальные параметры - извлекаются в конструкторе команды. Может все параметры брать в одном общем месте?

class SCPCommandAnswer : public SCPCommand {
public:
	SCPCommandAnswer() {}

	void process(unsigned int channel) override {
		// TODO: получать номер канала не через параметр?
		commands::Answer(channel);
	}

	std::string getCommandName() override {
		return "ANSWER";
	}

	class Factory : public SCPCommandFactory {
	public:
		std::unique_ptr<SCPCommand> Construct(std::istream& in) const override {
			return std::unique_ptr<SCPCommandAnswer>(new SCPCommandAnswer());
		}
	};

};

class SCPCommandDrop : public SCPCommand {
public:
	SCPCommandDrop(RedirectionReason reason) : reason_(reason) {};

	void process(unsigned int channel) override {
		// TODO: получать номер канала не через параметр?
		commands::DropCall(channel, reason_);
	}

	std::string getCommandName() override {
		return "DROP";
	}

	class Factory : public SCPCommandFactory {
	public:
		std::unique_ptr<SCPCommand> Construct(std::istream& in) const override {
			RedirectionReason reason;
			in.read(reinterpret_cast<char*>(&reason), sizeof(reason));
			return std::unique_ptr<SCPCommandDrop>(new SCPCommandDrop(reason));
		}
	};
private:
	RedirectionReason reason_;
};


class SCPCommandPlay : public SCPCommand {
public:
	SCPCommandPlay(const std::string& fragment) : fragmentName_(fragment) {};

	void process(unsigned int channel) override {
		// TODO: error handling
		commands::PlayFragment(channel, fragmentName_);
	}

	std::string getCommandName() override {
		return "PLAY";
	}

	class Factory : public SCPCommandFactory {
	public:
		std::unique_ptr<SCPCommand> Construct(std::istream& in) const override {
			std::string fragment;
			uint16_t size;
			in.read(reinterpret_cast<char*>(&size), sizeof(size));
			fragment.resize(size);
			in.read(&(fragment[0]), size);
			return std::unique_ptr<SCPCommandPlay>(new SCPCommandPlay(fragment));
		}
	};
private:
	std::string fragmentName_;
};


const SCPCommandFactory& SCPCommandFactory::GetFactory(int id) {
	static SCPCommandDrop::Factory dropFactory;
	static SCPCommandAnswer::Factory answerFactory;
	static SCPCommandPlay::Factory playFactory;

	static std::unordered_map<int, const SCPCommandFactory&> factories
		= { {SCPCommandCodes::CMD_DROP, dropFactory},  {SCPCommandCodes::CMD_ANSWER, answerFactory}, {SCPCommandCodes::CMD_PLAY, playFactory} };

	return factories.at(id);
}


} // namespace ssp_scp