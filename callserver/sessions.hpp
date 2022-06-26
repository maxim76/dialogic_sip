#pragma once
#include <limits>
#include <unordered_map>

#include "logging.h"

typedef uint32_t TSessionID;
const TSessionID UNKNOWN_SESSION = std::numeric_limits<TSessionID>::max();

class Session {
public:
	// TODO: сделать сессию уникальной даже если работает несколько приложений и на нескольких хостах
	static TSessionID getNewSessionID() {
		return lastSession++;
	}
private:
	static TSessionID lastSession;
};

class SessionList {
public:
	TSessionID createSession(int channel) {
		// TODO: проверить нету ли уже сессии на этом канале
		TSessionID session = Session::getNewSessionID();
		channel2session[channel] = session;
		session2channel[session] = channel;
		Log(TRC_CORE, TRC_DUMP, channel, "Created session %u on channel %i", session, channel);
		return session;
	}

	void deleteSession(int channel);
	TSessionID getSessionByChannel(int channel);
	int getChannelBySession(TSessionID session);
private:
	std::unordered_map<int, TSessionID> channel2session;
	std::unordered_map<TSessionID, int> session2channel;
};
