#include "pch.h"
#include "VectorLog.h"

using namespace oe;

void VectorLog::append(high_resolution_time_point timestamp, std::string&& text, int level)
{
    if (_messageCount == _maxMessages) {
        _firstMessageIndex++;
    }

    auto& message = _messages.at((_messageCount + _firstMessageIndex) % _maxMessages);
    message.timestamp = timestamp;
    message.message = text;
    message.level = level;

    if (_messageCount == _maxMessages) {
        _firstMessageIndex++;
        assert(_maxMessages == _messages.size());
    }
    else {
        _messageCount++;
    }
}
