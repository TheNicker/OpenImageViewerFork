#pragma once

#include <LWS/interfaces/backends.hpp>

#include <any>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

struct EventData
{
    uint16_t id;
    std::any data;
};

using OnMessageCallback = std::function<void(const EventData&)>;
using EventDataList     = std::vector<EventData>;

// Transitional application-thread dispatch queue. This should move to LWS once LWS exposes task dispatch.
class EventSync
{
  public:

    explicit EventSync(OnMessageCallback callback);
    ~EventSync();

    EventSync(const EventSync&)            = delete;
    EventSync& operator=(const EventSync&) = delete;

    [[nodiscard]] LWS::Handle GetEventHandle() const;

    template <typename T>
    void AddData(uint16_t id, T&& anyVar)
    {
        {
            std::lock_guard lock(fMutex);
            fSharedDataList.emplace_back(EventData{id, std::forward<T>(anyVar)});
        }
        Signal();
    }

    void ProcessData();

  private:

    struct NativeState;

    void Signal();
    void Reset();

    std::unique_ptr<NativeState> fNativeState;
    std::mutex fMutex;
    EventDataList fSharedDataList;
    OnMessageCallback fCallback;
};
