#pragma once

enum EventType {
    WINDOW_CLOSE = 0,
    KEY
};

class Event {
public:
    virtual EventType GetType() = 0;
    // virtual ~Event() = default;
};