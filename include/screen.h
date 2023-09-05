#pragma once

#include <functional>

class screen {
    public:
    screen(std::function<bool()> draw_func) : _draw(draw_func) {
    }

    bool draw() {
        if (_draw) {
            _draw();
            return true;
        }
        return false;
    }

    bool button1() {
        return false;
    }

    bool button2() {
        return false;
    }

    private:
    std::function<bool()> _draw;
};