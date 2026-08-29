#include "EventSync.h"

#ifdef LWS_PLATFORM_WAYLAND
    #include <LWS/Wayland/PlatformWayland.hpp>
#endif

#include <atomic>

struct EventSync::NativeState
{
    std::atomic_bool taskPending{};
};

EventSync::EventSync(OnMessageCallback callback)
    : fNativeState(std::make_unique<NativeState>()), fCallback(std::move(callback))
{
}

EventSync::~EventSync() = default;

LWS::Handle EventSync::GetEventHandle() const
{
    return 0;
}

void EventSync::Signal()
{
#ifdef LWS_PLATFORM_WAYLAND
    if (!fNativeState->taskPending.exchange(true))
        LWS::Wayland::PostTask([this] { ProcessData(); });
#endif
}

void EventSync::Reset()
{
    fNativeState->taskPending = false;
}
