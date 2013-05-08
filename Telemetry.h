// telemetry routines

#ifndef TELEMETRY_H
#define TELEMETRY_H

struct DetectionTimer
{
    DetectionTimer()
    {
        time = 0;
        detected = false;
    }

    bool Update( float dt, bool condition, float threshold = 0.25f )
    {
        if ( condition )
        {
            time += dt;
            if ( time > threshold )
            {
                if ( !detected )
                {
                    detected = true;
                    return true;
                }
            }
        }
        else
        {
            detected = false;
            time = 0;
        }

        return false;
    }

    float time;
    bool detected;
};

enum Counters
{
    COUNTER_ToggleLocked,

    COUNTER_PlacedStone,
    COUNTER_PlacedStoneHard,

    COUNTER_ZoomedIn,
    COUNTER_ZoomedOut,
    COUNTER_AppliedImpulse,
    COUNTER_Swiped,
    COUNTER_SelectedStone,
    COUNTER_DraggedStone,
    COUNTER_FlickedStone,
    COUNTER_TouchedStone,

    COUNTER_AtRestBoard,
    COUNTER_AtRestGroundPlane,
    COUNTER_AtRestLeftPlane,
    COUNTER_AtRestRightPlane,
    COUNTER_AtRestTopPlane,
    COUNTER_AtRestBottomPlane,
    COUNTER_AtRestNearPlane,

    COUNTER_SlidingBoard,
    COUNTER_SlidingGroundPlane,
    COUNTER_SlidingLeftPlane,
    COUNTER_SlidingRightPlane,
    COUNTER_SlidingTopPlane,
    COUNTER_SlidingBottomPlane,
    COUNTER_SlidingNearPlane,

    COUNTER_SpinningBoard,
    COUNTER_SpinningGroundPlane,
    COUNTER_SpinningLeftPlane,
    COUNTER_SpinningRightPlane,
    COUNTER_SpinningTopPlane,
    COUNTER_SpinningBottomPlane,
    COUNTER_SpinningNearPlane,

    COUNTER_OrientationPerfectlyFlat,
    COUNTER_OrientationNeutral,
    COUNTER_OrientationLeft,
    COUNTER_OrientationRight,
    COUNTER_OrientationUp,
    COUNTER_OrientationDown,
    COUNTER_OrientationUpsideDown,

    COUNTER_NumValues
};

const char * CounterNames[] = 
{
    "toggle locked",

    "placed stone",
    "placed stone hard",

    "zoomed in",
    "zoomed out",
    "applied impulse",
    "swiped",
    "selected stone",
    "dragged stone",
    "flicked stone",
    "touched stone",
    
    "at rest board",
    "at rest ground plane",
    "at rest left plane",
    "at rest right plane",
    "at rest top plane",
    "at rest bottom plane",
    "at rest near plane",
    
    "sliding board",
    "sliding ground plane",
    "sliding left plane",
    "sliding right plane",
    "sliding top plane",
    "sliding bottom plane",
    "sliding near plane",

    "spinning board",
    "spinning ground plane",
    "spinning left plane",
    "spinning right plane",
    "spinning top plane",
    "spinning bottom plane",
    "spinning near plane",

    "orientation perfectly flat",
    "orientation neutral",
    "orientation left",
    "orientation right",
    "orientation up",
    "orientation down",
    "orientation upside down"
};

enum Conditions
{
    CONDITION_AtRestBoard,
    CONDITION_AtRestGroundPlane,
    CONDITION_AtRestLeftPlane,
    CONDITION_AtRestRightPlane,
    CONDITION_AtRestTopPlane,
    CONDITION_AtRestBottomPlane,
    CONDITION_AtRestNearPlane,

    CONDITION_SlidingBoard,
    CONDITION_SlidingGroundPlane,
    CONDITION_SlidingLeftPlane,
    CONDITION_SlidingRightPlane,
    CONDITION_SlidingTopPlane,
    CONDITION_SlidingBottomPlane,
    CONDITION_SlidingNearPlane,

    CONDITION_SpinningBoard,
    CONDITION_SpinningGroundPlane,
    CONDITION_SpinningLeftPlane,
    CONDITION_SpinningRightPlane,
    CONDITION_SpinningTopPlane,
    CONDITION_SpinningBottomPlane,
    CONDITION_SpinningNearPlane,

    CONDITION_OrientationPerfectlyFlat,
    CONDITION_OrientationNeutral,
    CONDITION_OrientationLeft,
    CONDITION_OrientationRight,
    CONDITION_OrientationUp,
    CONDITION_OrientationDown,
    CONDITION_OrientationUpsideDown,

    CONDITION_NumValues
};

enum CollisionPlanes
{
    COLLISION_Board,
    COLLISION_GroundPlane,
    COLLISION_LeftPlane,
    COLLISION_RightPlane,
    COLLISION_TopPlane,
    COLLISION_BottomPlane,
    COLLISION_NearPlane,
    COLLISION_NumValues
};

#endif
