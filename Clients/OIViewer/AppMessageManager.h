#pragma once
#include <Windows.h>
#include <functional>
#include <cstdint>
#include <any>
#include <LLUtils/UniqueIDProvider.h>
#include <mutex>

namespace OIV
{

using MessageType = uint32_t;
using MessageCallBackType = std::function<void(Message)>;

struct Message
{
    MessageType type;
    size_t size;
    std::any data;
};

using MessagesType = std::list<Message>;

class AppMessageManager
{
    public:
    AppMessageManager()
    {
        fEventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    /// @brief register a message handler
    /// @param  
    /// @return 
    MessageType AddMessageHandler(MessageCallBackType)
    {
        fIDProvider.Acquire();
    }
   
   void QueueMessage(MessageType type,size_t size, std::any data)
   {
    MessagesType messagesToFlush;
        {
            std::swap(messagesToFlush, fMessages);
        }

        for (auto& message : messagesToFlush)
        {
            fMessageCallBack(message.type, message.size, message.data);
        }
    std::lock_guard<std::mutex> lock(fMessageSyncMutex);

   }

   void SetEvent()
   {
        ::SetEvent(fEventHandle);
   }


   void HandleEvents()
   {
        DWORD result = MsgWaitForMultipleObjectsEx(1, &fEventHandle, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE);
        if (result == WAIT_OBJECT_0)
        { 
            //flush queued messages at the desired thread
            
            ::ResetEvent(fEventHandle);
        }
   }
    HANDLE fEventHandle{};
    LLUtils::UniqueIdProvider <MessageType> fIDProvider;
    std::mutex fMessageSyncMutex;
    std::list<Message> fMessages;
 };
 
}