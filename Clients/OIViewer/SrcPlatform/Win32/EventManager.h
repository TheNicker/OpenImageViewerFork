#pragma once
#include <LLUtils/Event.h>
#include <LLUtils/Singleton.h>
#include <LWS/Platform.hpp>

namespace OIV
{
    class EventManager : public LLUtils::Singleton<EventManager>
    {
    public:

        struct MonitorChangeEventParams
        {
            LWS::Platform::MonitorDesc monitorDesc;
        };

        using MonitorChangeEvent = LLUtils::Event<void(const MonitorChangeEventParams&)>;
        
        MonitorChangeEvent MonitorChange;


        struct SizeChangeEventParams
        {
            int32_t width;
            int32_t height;
            
        };

        using SizeChangeEvent = LLUtils::Event<void(const SizeChangeEventParams&)>;

        SizeChangeEvent SizeChange;

        
    };
}
