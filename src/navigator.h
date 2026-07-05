#pragma once
#include <Arduino.h>
#include "config.h"
#include "sensor_array.h"
#include "encoder.h"
#include "pid_controller.h"

// Motion planner for the PRIM.E single-path maze.
//
// The corridor is 230 mm wide with no dead ends or decision junctions,
// and any wall touch forfeits the current section's points — so the
// planner's whole job is: stay centered, spot the corner early via the
// diagonal sensors, brake on a v² profile, pivot 90°, resume.
class Navigator {
public:
    enum Phase { FOLLOW, BRAKE_FOR_TURN, PIVOT, EXIT_TURN, FINISHED };

    void begin(EncoderSystem* enc);
    void reset();

    // Called at 100 Hz. Produces target wheel speeds (mm/s).
    void plan(const SensorData& s, float cruiseSpeed);

    float leftTarget()  const { return _targetL; }
    float rightTarget() const { return _targetR; }
    bool  wantsShortBrake() const { return _shortBrake; }
    Phase phase() const { return _phase; }

private:
    void  startPivot(bool leftTurn);
    float brakeProfileSpeed(float distToWall, float floor) const;

    EncoderSystem* _enc = nullptr;
    PIDController  _steerPID{STEER_KP, STEER_KI, STEER_KD, -180.0f, 180.0f};

    Phase _phase = FOLLOW;
    bool  _pivotLeft = false;
    long  _pivotStartL = 0, _pivotStartR = 0;
    long  _pivotCounts = 0;
    float _exitDistStart = 0;
    float _targetL = 0, _targetR = 0;
    bool  _shortBrake = false;
    uint32_t _openBothSince = 0;   // finish heuristic timer
};
