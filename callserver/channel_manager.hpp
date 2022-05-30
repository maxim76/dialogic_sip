#pragma once

class ChannelManager {
public:
//	ChannelManager(T_CHAN_INFO* channel_info_ptr);
private:
//	T_CHAN_INFO* ChannelInfo;
};

// TODO: сделать класс обрабатывающий эти команды и хранящий все что касается Dialogic
namespace commands {

void Answer(int channel);
void PlayFragment(int index, const std::string& fragment);
void DropCall(int channel, int reason);

} // namespace commands
