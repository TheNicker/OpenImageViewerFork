#pragma once
#include <Windows.h>
#include <functional>
#include <cstdint>
#include <any>
#include <LLUtils/UniqueIDProvider.h>
#include <mutex>
#include <map>

namespace OIV
{

    using MessageType = uint32_t;

    struct Message
    {
        MessageType type;
        size_t size;
        std::any data;
    };

    using MessageCallBackType = std::function<void(Message)>;

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
        MessageType AddMessageHandler(MessageCallBackType callback)
        {
            auto messageType = fIDProvider.Acquire();
            fMessageHandlers.emplace(messageType, callback);
            return messageType;
        }

        void QueueMessage(MessageType type, size_t size, std::any data)
        {
            {
                std::lock_guard<std::mutex> lock(fMessageSyncMutex);
                fMessages.emplace_back(type, size, data);
            }
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
                MessagesType messagesToFlush;
                {
                    std::lock_guard<std::mutex> lock(fMessageSyncMutex);
                    std::swap(messagesToFlush, fMessages);
                }

                // flush queued messages at the desired thread
                for (auto &message : messagesToFlush)
                {
                    auto it = fMessageHandlers.find(message.type);
                    it->second(message);
                }
                ::ResetEvent(fEventHandle);
            }
        }
        HANDLE fEventHandle{};
        LLUtils::UniqueIdProvider<MessageType> fIDProvider;
        std::mutex fMessageSyncMutex;
        std::list<Message> fMessages;
        std::map<MessageType, MessageCallBackType> fMessageHandlers;
    };

}