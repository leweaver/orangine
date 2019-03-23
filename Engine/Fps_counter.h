#pragma once

namespace oe {
    class Perf_timer;

    class Fps_counter {
    public:
        explicit Fps_counter(int bufferSize = 30);
        ~Fps_counter();

        void mark();

        // Running average frame time, in seconds
        float avgFrameTime() const;

        float avgFps() const;

    private:
        std::unique_ptr<Perf_timer> _perfTimer;
        std::vector<double> _frameTimes;
        std::size_t _lastFrameTimeIdx = 0;
        double _frameTimeTotal = 0.0;
        int _bufferSize;
    };
}