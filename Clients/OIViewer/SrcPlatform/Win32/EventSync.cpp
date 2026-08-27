#include "EventSync.h"

#include <LLUtils/Exception.h>

#include <Windows.h>

struct EventSync::NativeState
{
    NativeState() : event(CreateEvent(nullptr, TRUE, FALSE, nullptr)) {}

    ~NativeState()
    {
        if (event != nullptr)
            CloseHandle(event);
    }

    HANDLE event = nullptr;
};

EventSync::EventSync(OnMessageCallback callback)
    : fNativeState(std::make_unique<NativeState>()), fCallback(std::move(callback))
{
    if (fNativeState->event == nullptr)
        LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Failed to create event");
}

EventSync::~EventSync() = default;

LWS::Handle EventSync::GetEventHandle() const
{
    return reinterpret_cast<LWS::Handle>(fNativeState->event);
}

void EventSync::Signal()
{
    if (!SetEvent(fNativeState->event))
        LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Failed to set event");
}

void EventSync::Reset()
{
    if (!ResetEvent(fNativeState->event))
        LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Failed to reset event");
}
