#include "pch.h"
#include "Fps_counter.h"
#include "Perf_timer.h"

using namespace oe;

Fps_counter::Fps_counter(int bufferSize)
    : _bufferSize(bufferSize)
{
    assert(bufferSize > 0);
    _perfTimer = std::make_unique<Perf_timer>();
}

Fps_counter::~Fps_counter()
{
    _perfTimer.reset();
}

void Fps_counter::mark()
{
    if (_frameTimes.empty()) {
        _frameTimes.resize(_bufferSize);
        _perfTimer->restart();
    }

    _perfTimer->stop();

    _lastFrameTimeIdx = (_lastFrameTimeIdx + 1) % _bufferSize;
    _frameTimeTotal -= _frameTimes[_lastFrameTimeIdx];
    const auto elapsed = _perfTimer->elapsedSeconds();
    _frameTimeTotal += elapsed;
    _frameTimes[_lastFrameTimeIdx] = elapsed;

    _perfTimer->restart();
}

float Fps_counter::avgFrameTime() const
{
    return _frameTimeTotal / _bufferSize;
}

float Fps_counter::avgFps() const
{
    if (_frameTimeTotal <= 0.0)
        return 0.0;
    return 1.0 / (_frameTimeTotal / _bufferSize);
}
