#include <fstream>
#include <sstream>

#include "utils.h"

template <class Rep, class Period>
double duration2secs(chrono::duration<Rep, Period> dura)
{
    return chrono::duration_cast<chrono::milliseconds>(dura).count() / 1e3;
}

void FPSCounter::record()
{
    auto now = Clock::now();
    while (!_que.empty() && duration2secs(now - _que.front()) >= recordTime + Eps)
    {
        _que.pop();
        --_cnt;
    }
    _que.push(now);
    ++_cnt;
}
double FPSCounter::getFPS()
{
    return static_cast<double>(_cnt) / recordTime;
}

ScopeGuard::ScopeGuard(std::function<void()> func) noexcept : _func(func)
{
}
ScopeGuard::ScopeGuard(ScopeGuard &&other) : _func(other._func), _commit(other._commit)
{
    other._commit = true;
}
ScopeGuard &ScopeGuard::operator=(ScopeGuard &&other)
{
    _func = other._func;
    _commit = other._commit;
    other._commit = true;
    return *this;
}
ScopeGuard::~ScopeGuard()
{
    if (!_commit)
        _func();
}

void ScopeGuard::commit()
{
    _commit = true;
}

std::string loadFile(const std::string &fileName)
{
    std::ifstream fin(fileName);
    std::stringstream ss;
    ss << fin.rdbuf();
    return ss.str();
}
