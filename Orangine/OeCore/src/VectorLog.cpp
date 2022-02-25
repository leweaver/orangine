#include "OeCore/VectorLog.h"

using namespace oe;

void VectorLog::append(high_resolution_time_point timestamp, std::string&& text, int level)
{
    auto& message = _messages.at((_messageCount + _firstMessageIndex) % _maxMessages);
    message.timestamp = timestamp;
    message.message = text;
    message.level = level;

    if (_messageCount == _maxMessages) {
        _firstMessageIndex = (_firstMessageIndex + 1) % _maxMessages;
    }
    else {
        ++_messageCount;
    }
}

void VectorLog::clear() {
  _messageCount = 0;
  _firstMessageIndex = 0;
  std::fill(_messages.begin(), _messages.end(), Log_message());
}
