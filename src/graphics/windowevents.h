#pragma once

#include "core/event.h"

class WindowCloseEvent : public Event {
public:
    WindowCloseEvent() {};
    ~WindowCloseEvent() {};

    virtual EventType GetType() {
        return EventType::WINDOW_CLOSE;
    }
};

class KeyEvent : public Event {
public:
    KeyEvent(int key, int action): key(key), action(action) {}
    ~KeyEvent() {}

    virtual EventType GetType() {
        return EventType::KEY;
    }

    int key;
    int action;
};