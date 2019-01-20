#pragma once

#include <vector>
#include "g3log/logmessage.hpp"

namespace oe {
    class VectorLogSink {
    public:
        typedef std::chrono::time_point<std::chrono::high_resolution_clock> high_resolution_time_point;
        struct Log_message {

            high_resolution_time_point timestamp;
            std::string message;
            int level;
        };

        explicit VectorLogSink(uint32_t maxMessages)
            : _maxMessages(maxMessages)
            , _firstMessageIndex(0)
            , _messageCount(0)
        {}

        void append(g3::LogMessageMover messageMover);

        uint32_t messageCount() const { return _messageCount; }
        const Log_message& getMessageAt(uint32_t index) { return _messages.at((_firstMessageIndex + index) % _maxMessages); }

    private:
        // Oldest message is earliest in the array.
        std::vector<Log_message> _messages;

        const uint32_t _maxMessages;
        uint32_t _firstMessageIndex;
        uint32_t _messageCount;
    };
}