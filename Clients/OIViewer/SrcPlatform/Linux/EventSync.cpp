#include "EventSync.h"

#include <LLUtils/Exception.h>

struct EventSync::NativeState
{
};

EventSync::EventSync(OnMessageCallback callback)
    : fNativeState(std::make_unique<NativeState>()), fCallback(std::move(callback))
{
}

EventSync::~EventSync() = default;

LWS::Handle EventSync::GetEventHandle() const
{
    LL_EXCEPTION_NOT_IMPLEMENT("Native event waiting is not implemented on Linux");
}

void EventSync::Signal() {}
void EventSync::Reset() {}
