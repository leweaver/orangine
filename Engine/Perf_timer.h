#pragma once

namespace oe {
    class Perf_timer {
    public:

        explicit Perf_timer(long long startTime = 0) 
            : _startTime(startTime)
            , _endTime(0)
        {
        }

        long long static currentCounter()
        {
            return _Query_perf_counter();
        }

        static Perf_timer start()
        {
            return Perf_timer(currentCounter());
        }

        void restart()
        {
            _startTime = currentCounter();
        }

        void stop()
        {
            _endTime = currentCounter();
        }

        long long elapsedRaw() const
        {
            assert(_endTime >= _startTime);
            return _endTime - _startTime;
        }

        double elapsedSeconds() const
        {
            assert(_endTime >= _startTime);
            return static_cast<double>(_endTime - _startTime) / static_cast<double>(_Query_perf_frequency());
        }

    private:

        long long _startTime;
        long long _endTime;
    };
}
