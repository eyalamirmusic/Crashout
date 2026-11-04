#include "CrashReportingPlatform.h"

#include <CoreFoundation/CoreFoundation.h>

#include <array>
#include <climits>

namespace eacp::CrashReporting::Detail
{
namespace
{
// Waiting begins after every other observer's work before the wait, and ends
// before any observer's work after it, so none of that work is taken for idle.
constexpr auto lastObserverOrder = CFIndex {LONG_MAX};
constexpr auto firstObserverOrder = CFIndex {LONG_MIN};

struct EventWaitObserver
{
    std::array<CFRunLoopObserverRef, 2> observers {};
};

EventWaitObserver& getEventWaitObserver()
{
    static auto observer = EventWaitObserver {};
    return observer;
}

void handleBeforeWaiting(CFRunLoopObserverRef, CFRunLoopActivity, void* context)
{
    beginWaitingForEvents(*static_cast<SharedWatchedThread*>(context));
}

void handleAfterWaiting(CFRunLoopObserverRef, CFRunLoopActivity, void* context)
{
    endWaitingForEvents(*static_cast<SharedWatchedThread*>(context));
}

CFRunLoopObserverRef addObserver(SharedWatchedThread& thread,
                                 CFRunLoopActivity activity,
                                 CFIndex order,
                                 CFRunLoopObserverCallBack callback)
{
    constexpr auto repeats = true;
    auto context = CFRunLoopObserverContext {0, &thread, nullptr, nullptr, nullptr};
    auto* observer = CFRunLoopObserverCreate(
        kCFAllocatorDefault, activity, repeats, order, callback, &context);
    CFRunLoopAddObserver(CFRunLoopGetCurrent(), observer, kCFRunLoopCommonModes);
    return observer;
}
} // namespace

void observeEventWaitsOnCurrentThread(SharedWatchedThread& thread)
{
    auto& waits = getEventWaitObserver();

    if (waits.observers[0] != nullptr)
        return;

    waits.observers = {
        addObserver(
            thread, kCFRunLoopBeforeWaiting, lastObserverOrder, handleBeforeWaiting),
        addObserver(
            thread, kCFRunLoopAfterWaiting, firstObserverOrder, handleAfterWaiting)};
}

void stopObservingEventWaits()
{
    auto& waits = getEventWaitObserver();

    if (waits.observers[0] == nullptr)
        return;

    for (auto* observer: waits.observers)
    {
        CFRunLoopObserverInvalidate(observer);
        CFRelease(observer);
    }

    waits = {};
}
} // namespace eacp::CrashReporting::Detail
