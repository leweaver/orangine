#include "pch.h"
#include "VectorLogSink.h"

void oe::VectorLogSink::append(g3::LogMessageMover messageMover)
{
    if (_messageCount == _maxMessages) {
        _firstMessageIndex++;
    }

    auto& message = _messages.at((_messageCount + _firstMessageIndex) % _maxMessages);
    message.timestamp = messageMover.get()._timestamp;
    message.message = messageMover.get().message();
    message.level = messageMover.get()._level.value;

    if (_messageCount == _maxMessages) {
        _firstMessageIndex++;
        assert(_maxMessages == _messages.size());
    } else {
        _messageCount++;
    }
}
