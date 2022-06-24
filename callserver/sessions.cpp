#include "sessions.hpp"

TSessionID Session::lastSession = 0;

int SessionList::getChannelBySession(TSessionID session) {
	auto it = session2channel.find(session);
	if (it != session2channel.end()) {
		return it->second;
	}
	return -1;
}

TSessionID SessionList::getSessionByChannel(int channel) {
	auto it = channel2session.find(channel);
	if (it != channel2session.end()) {
		return it->second;
	}
	return UNKNOWN_SESSION;
}
