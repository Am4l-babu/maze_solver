#include "navigator.h"

void Navigator::begin(EncoderSystem* enc) {
    _enc = enc;
    reset();
}

void Navigator::reset() {
    _phase = FOLLOW;
    _targetL = _targetR = 0;
    _shortBrake = false;
    _openBothSince = 0;
    _steerPID.reset();
}

// v = sqrt(2·a·d): the speed from which we can still stop in d.
float Navigator::brakeProfileSpeed(float distToWall, float floor) const {
    float d = distToWall - STOP_THRESHOLD_MM;
    if (d <= 0) return 0;
    float v = sqrtf(2.0f * MAX_DECEL_MM_S2 * d);
    return max(v, floor);
}

void Navigator::startPivot(bool leftTurn) {
    _pivotLeft = leftTurn;
    _pivotStartL = _enc->getLeftCount();
    _pivotStartR = _enc->getRightCount();
    // Arc length per wheel for a 90° in-place pivot: (π/4)·wheelbase
    float arcMM = (PI / 4.0f) * WHEELBASE_MM;
    _pivotCounts = (long)(arcMM / MM_PER_COUNT);
    _phase = PIVOT;
}

void Navigator::plan(const SensorData& s, float cruiseSpeed) {
    _shortBrake = false;
    const float dt = LOOP_PERIOD_MS / 1000.0f;

    switch (_phase) {

    case FOLLOW: {
        bool wallL = s.left  < WALL_PRESENT_MM;
        bool wallR = s.right < WALL_PRESENT_MM;
        bool frontClosing = s.front < (TURN_TRIGGER_MM + 160.0f);

        // Finish heuristic: corridor fully opens and stays open — we've
        // exited the maze at the bottom-right corner.
        if (!wallL && !wallR && s.front > OPENING_MM) {
            if (_openBothSince == 0) _openBothSince = millis();
            if (millis() - _openBothSince > 600) { _phase = FINISHED; break; }
        } else {
            _openBothSince = 0;
        }

        // Corner detection. Single-path track: when the front wall
        // closes in, exactly one side will be open — that's the turn.
        if (frontClosing) {
            _phase = BRAKE_FOR_TURN;
            break;
        }

        // Corridor centering. Prefer both walls; fall back to
        // single-wall offset hold when one side is open (pre-corner).
        float error;                              // +ve = too far right
        if (wallL && wallR)      error = (s.left - s.right) * 0.5f;
        else if (wallL)          error = s.left  - CENTER_SIDE_DIST_MM;
        else if (wallR)          error = CENTER_SIDE_DIST_MM - s.right;
        else                     error = 0;

        // Diagonals catch approach-angle drift before the 90° sensors do.
        if (s.diagL < WALL_PRESENT_MM && s.diagR < WALL_PRESENT_MM)
            error += 0.35f * (s.diagL - s.diagR);

        float steer = _steerPID.compute(0.0f, -error, dt);

        // Slow down when off-center: speed costs points only via wall hits.
        float centerFactor = constrain(1.0f - fabsf(error) / 60.0f, 0.45f, 1.0f);
        float v = cruiseSpeed * centerFactor;

        _targetL = v + steer;
        _targetR = v - steer;
        break;
    }

    case BRAKE_FOR_TURN: {
        float v = brakeProfileSpeed(s.front, 60.0f);
        v = min(v, TURN_APPROACH_MM_S);

        // Keep centering while braking (reduced authority).
        float error = 0;
        if (s.left < WALL_PRESENT_MM && s.right < WALL_PRESENT_MM)
            error = (s.left - s.right) * 0.5f;
        float steer = 0.5f * _steerPID.compute(0.0f, -error, dt);

        _targetL = v + steer;
        _targetR = v - steer;

        if (s.front <= TURN_TRIGGER_MM) {
            bool openL = s.left  > OPENING_MM || s.diagL > OPENING_MM;
            bool openR = s.right > OPENING_MM || s.diagR > OPENING_MM;
            if (openL || openR) {
                _shortBrake = true;              // kill remaining momentum
                startPivot(openL && !openR ? true
                          : (!openL && openR ? false
                          : (s.left > s.right)));  // both open: pick wider
            } else {
                // No opening yet (sensor lag) — creep until one appears.
                _targetL = _targetR = 60.0f;
            }
        }
        break;
    }

    case PIVOT: {
        long dL = labs(_enc->getLeftCount()  - _pivotStartL);
        long dR = labs(_enc->getRightCount() - _pivotStartR);
        long progress = (dL + dR) / 2;

        if (progress >= _pivotCounts) {
            _shortBrake = true;
            _steerPID.reset();
            _exitDistStart = (_enc->getLeftDistance() + _enc->getRightDistance()) * 0.5f;
            _phase = EXIT_TURN;
            break;
        }
        // Taper pivot speed over the last 30% for a clean stop.
        float frac = (float)progress / (float)_pivotCounts;
        float w = (frac > 0.7f) ? 90.0f : 180.0f;   // wheel mm/s
        _targetL = _pivotLeft ? -w :  w;
        _targetR = _pivotLeft ?  w : -w;
        break;
    }

    case EXIT_TURN: {
        // Drive straight ~60 mm on encoders before trusting the side
        // sensors again (they sweep across the corner opening).
        float travelled = (_enc->getLeftDistance() + _enc->getRightDistance()) * 0.5f
                          - _exitDistStart;
        _targetL = _targetR = TURN_APPROACH_MM_S;
        if (travelled > 60.0f) _phase = FOLLOW;
        break;
    }

    case FINISHED:
        _targetL = _targetR = 0;
        _shortBrake = true;
        break;
    }
}
