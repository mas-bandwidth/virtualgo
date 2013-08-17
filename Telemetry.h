// telemetry routines

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <vector>
#include "Board.h"
#include "StoneInstance.h"

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

typedef void (*CounterNotifyFunc)( int counterIndex, uint64_t counterValue, const char * counterName );

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

enum Collisions
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

class Telemetry
{
public:

    Telemetry()
    {
        collisionMask = 0;
        swipedThisFrame = false;
        secondsSinceLastSwipe = 0;
        memset( counters, 0, sizeof( counters ) );
        memset( secondsSinceCollision, 0, sizeof( secondsSinceCollision ) );
        counterNotifyFunc = NULL;
    }

    void SetCounterNotifyFunc( CounterNotifyFunc function )
    {
        counterNotifyFunc = function;
    }

    void Update( float dt, const Board & board, const std::vector<StoneInstance> & stones, bool locked, const vec3f & up )
    {
        // todo: convert this to work with new telemetry system

        const StoneInstance & stone = stones[0];

        // update seconds since last swipe

        if ( swipedThisFrame )
            secondsSinceLastSwipe = 0;
        else
            secondsSinceLastSwipe += dt;

        // update seconds since collision

        bool recentCollision[COLLISION_NumValues];

        for ( int i = 0; i < COLLISION_NumValues; ++i )
        {
            secondsSinceCollision[i] += dt;

            if ( collisionMask == i )
                secondsSinceCollision[i] = 0;

            recentCollision[i] = secondsSinceCollision[i] < 0.1f;
        }

        // detect at rest on each collision plane and board

        const bool atRest = length( stone.rigidBody.linearVelocity ) < 1.0f &&
                            length( stone.rigidBody.angularVelocity ) < 0.1f;

        for ( int i = 0; i < COLLISION_NumValues; ++i )
        {
            const bool detected = detectionTimer[CONDITION_AtRestBoard+i].Update( dt, atRest && collisionMask == i );
            if ( detected )
                IncrementCounter( (Counters) ( COUNTER_AtRestBoard + i ) );
        }

        // detect sliding on each collision plane

        const float SlideTime = 0.5f;

        const bool sliding = !atRest &&
                             length( stone.rigidBody.linearVelocity ) < 10.0f &&
                             length( stone.rigidBody.angularVelocity ) < 1.0f;

        for ( int i = 0; i < COLLISION_NumValues; ++i )
        {
            const bool detected = detectionTimer[CONDITION_SlidingBoard+i].Update( dt, sliding && collisionMask == i, SlideTime );
            if ( detected )
                IncrementCounter( (Counters) ( COUNTER_SlidingBoard + i ) );
        }

        // detect spinning on each collision plane

        const float SpinTime = 0.25f;

        vec3f stoneUp;
        stone.rigidBody.transform.GetUp( stoneUp );
        
        const float upSpin = fabs( dot( stone.rigidBody.angularVelocity, up ) );
        const float totalSpin = length( stone.rigidBody.angularVelocity );
        
        const bool spinning = length( stone.rigidBody.linearVelocity ) < 10.0 &&
                              dot( stoneUp, up ) < 0.25f &&
                              upSpin > 1.0f && fabs( upSpin / totalSpin ) > 0.7f &&
                              secondsSinceLastSwipe < 1.0f;

        for ( int i = 0; i < COLLISION_NumValues; ++i )
        {
            const bool detected = detectionTimer[CONDITION_SpinningBoard+i].Update( dt, spinning && recentCollision[i], SpinTime );
            if ( detected )
                IncrementCounter( (Counters) ( COUNTER_SpinningBoard + i ) );
        }

        // update orientation detection

        const float OrientationTime = 1.0f;

        if ( detectionTimer[CONDITION_OrientationPerfectlyFlat].Update( dt, dot( up, vec3f(0,0,1) ) > 0.99f, OrientationTime ) )
            IncrementCounter( COUNTER_OrientationPerfectlyFlat );
        
        if ( detectionTimer[CONDITION_OrientationNeutral].Update( dt, dot( up, vec3f(0,0,1) ) > 0.75f, OrientationTime ) )
            IncrementCounter( COUNTER_OrientationNeutral );

        if ( detectionTimer[CONDITION_OrientationLeft].Update( dt, dot( up, vec3f(1,0,0) ) > 0.75f, OrientationTime ) )
            IncrementCounter( COUNTER_OrientationLeft );

        if ( detectionTimer[CONDITION_OrientationRight].Update( dt, dot( up, vec3f(-1,0,0) ) > 0.75f, OrientationTime ) )
            IncrementCounter( COUNTER_OrientationRight );

        if ( detectionTimer[CONDITION_OrientationUp].Update( dt, dot( up, vec3f(0,1,0) ) > 0.75f, OrientationTime ) )
            IncrementCounter( COUNTER_OrientationUp );
        
        if ( detectionTimer[CONDITION_OrientationDown].Update( dt, dot( up, vec3f(0,-1,0) ) > 0.75f, OrientationTime ) )
            IncrementCounter( COUNTER_OrientationDown );

        if ( detectionTimer[CONDITION_OrientationUpsideDown].Update( dt, dot( up, vec3f(0,0,-1) ) > 0.75f, OrientationTime ) )
            IncrementCounter( COUNTER_OrientationUpsideDown );

        // clear values this frame

        swipedThisFrame = false;

        collisionMask = 0;
    }

    void SetSwipedThisFrame()
    {
        swipedThisFrame = true;
    }

    void SetCollision( uint32_t collision )
    {
        collisionMask |= collision;
    }

    void IncrementCounter( Counters counterIndex )
    {
        ++counters[counterIndex];
        if ( counterNotifyFunc )
            counterNotifyFunc( counterIndex, counters[counterIndex], CounterNames[counterIndex] );
    }

private:

    bool swipedThisFrame;
    double secondsSinceLastSwipe;
    uint32_t collisionMask;

    double secondsSinceCollision[COLLISION_NumValues];

    uint64_t counters[COUNTER_NumValues];
    
    DetectionTimer detectionTimer[CONDITION_NumValues];
    
    CounterNotifyFunc counterNotifyFunc;
};

#endif
