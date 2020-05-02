#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <cassert>
#include <chrono>
#include <functional>
#include <queue>
#include <string>

const double Eps = 1e-5;

// Time
namespace chrono = std::chrono;
using Clock = chrono::steady_clock;
using TimePoint = Clock::time_point;
template <class Rep, class Period>
double duration2secs(chrono::duration<Rep, Period> dura);

class FPSCounter
{
public:
    void record();
    double getFPS();

    static const double recordTime;

private:
    std::queue<TimePoint> _que;
    int _cnt{0};
};

// Scope
class ScopeGuard
{
public:
    ScopeGuard(std::function<void()> func) noexcept;
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard &operator=(const ScopeGuard &) = delete;
    ScopeGuard(ScopeGuard &&other);
    ScopeGuard &operator=(ScopeGuard &&other);
    ~ScopeGuard();

    void commit();

private:
    std::function<void()> _func;
    bool _commit = false;
};

// File
std::string loadFile(const std::string &fileName);

#endif