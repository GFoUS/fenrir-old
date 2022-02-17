#pragma once

#include "Core.h"

namespace GHUA {
    enum EventType {

    };
    
    enum EventFilter {
        INPUT = BIT(0)
    };

    struct Event {
        EventType type;
        EventFilter filters;

        
    };
}