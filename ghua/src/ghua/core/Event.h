#pragma once

#include "Core.h"
#include "spdlog/fmt/ostr.h"

namespace GHUA {
    enum EventType {

    };
    
    enum EventFilter {
        INPUT = BIT(0)
    };

    struct Event {
        EventType type;
        EventFilter filters;

        virtual inline const char* GetName() const = 0; 

        template<typename OStream>
        friend OStream &operator<<(OStream &os, const Event &c)
        {
            return os << c.GetName();
        }
    };
}