#pragma once
#include <Arduino.h>

enum RobotState {
    STATE_IDLE,         // waiting for start button
    STATE_CALIBRATING,  // zero encoders, sensor offsets, battery check
    STATE_RUNNING,      // single-path run (PRIM.E) or exploration (classic)
    STATE_SOLVING,      // classic mode only: flood fill + path extraction
    STATE_FAST_RUN,     // classic mode only: optimal-path sprint
    STATE_FINISHED,     // goal reached, motors braked
    STATE_FAULT         // low battery / sensor loss — safe stop
};

class StateMachine {
public:
    RobotState getState() const { return _state; }
    uint32_t   timeInState() const { return millis() - _enteredAt; }

    void setState(RobotState s) {
        if (s == _state) return;
        _state = s;
        _enteredAt = millis();
    }

    // Debounced start-button edge; returns true once per press.
    bool startPressed(bool rawPressed) {
        uint32_t now = millis();
        if (rawPressed && !_btnWas && (now - _btnAt) > 50) {
            _btnWas = true;
            _btnAt = now;
            return true;
        }
        if (!rawPressed) _btnWas = false;
        return false;
    }

private:
    RobotState _state = STATE_IDLE;
    uint32_t   _enteredAt = 0;
    bool       _btnWas = false;
    uint32_t   _btnAt = 0;
};
