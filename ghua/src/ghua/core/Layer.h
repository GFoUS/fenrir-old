#pragma once

#include "Core.h"
#include "Event.h"

#define EVENT_PUSH_CALLBACK  std::function<void(Event*)>

namespace GHUA
{
    enum HandleResult {
        CONTINUE = 0,
        STOP = 1
    };

    class Layer {
    public:
        virtual HandleResult HandleEvent(Event* e) = 0;
        virtual inline const char* GetName() const = 0; 
        Layer();
        ~Layer();

        inline void SetEventPushCallback(EVENT_PUSH_CALLBACK PushEvent) {
            this->PushEvent = PushEvent;
        };

        template<typename OStream>
        friend OStream &operator<<(OStream &os, const Layer &c)
        {
            return os << c.GetName();
        }
    protected:
        EVENT_PUSH_CALLBACK PushEvent;
    };
}
