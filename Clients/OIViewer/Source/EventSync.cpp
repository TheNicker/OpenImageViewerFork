#include "EventSync.h"

void EventSync::ProcessData()
{
    EventDataList sharedData;
    {
        std::lock_guard lock(fMutex);
        sharedData = std::move(fSharedDataList);
        Reset();
    }

    for (const auto& data : sharedData)
        fCallback(data);
}
