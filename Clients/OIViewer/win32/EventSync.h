#pragma once

#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include <any>

struct SharedData
{
    uint16_t id;
    std::any data;
};

using OnMessageCallback = std::function<void(const SharedData&)>;

using SharedDataList = std::vector<SharedData>;
using lock_guard = std::lock_guard<std::mutex>;

class EventSync
{
  public:

    // Constructor: Creates the event
    EventSync(OnMessageCallback callback) : hEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr)), fCallback(callback)
    {
        if (!hEvent)
        {
            // TODO: error
        }
    }

    // Destructor: Closes event handle
    ~EventSync()
    {
        if (hEvent)
        {
            CloseHandle(hEvent);
        }
    }

    // Get the event handle (for use in MsgWaitForMultipleObjects)
    const HANDLE& GetEventHandle() const
    {
        return hEvent;
    }

    // Add shared data to the list
    void AddSharedData(uint16_t id, std::any anyVar)
    {
        SharedData newData{id, anyVar};
        {
            lock_guard lock(mtx);
            sharedDataList.push_back(std::move(newData));
            SetEventSignal();
        }
    }
    /// @brief Process the data from the main thread, and call the callback function
    void ProcessData()
    {
        SharedDataList sharedData;
        {
            lock_guard lock(mtx);
            sharedData = std::move(sharedDataList);
            ResetEventSignal();
        }

        for (const auto& data : sharedData)
        {
            fCallback(data);
        }
    }

  private:

    // Set (Signal) the event
    void SetEventSignal()
    {
        if (!SetEvent(hEvent))
        {
            // TODO: error
        }
    }

    // Reset the event (only for manual-reset events)
    void ResetEventSignal()
    {
        if (!ResetEvent(hEvent))
        {
            // TODO: error
        }
    }

    HANDLE hEvent;                  // Event handle
    std::mutex mtx;                 // Mutex for data synchronization
    SharedDataList sharedDataList;  // List of SharedData objects
    OnMessageCallback fCallback;    // Callback function
};
