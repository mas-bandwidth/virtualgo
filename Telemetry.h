// telemetry routines

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <vector>
#include "Stone.h"
#include "Board.h"

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
    }

    void Update( const Board & board, const std::vector<Stone> & stones, float dt, bool locked, const vec3f & up )
    {
    #ifndef MULTIPLE_STONES

        const Stone & stone = stones[0];

        // update seconds since last swipe

        if ( swipedThisFrame )
            secondsSinceLastSwipe = 0;
        else
            secondsSinceLastSwipe += dt;

        // update seconds since collision

        for ( int i = 0; i < COLLISION_NumValues; ++i )
            secondsSinceCollision[i] += dt;

        if ( collisionMask == COLLISION_Board )
            secondsSinceCollision[COLLISION_Board] = 0;

        if ( collisionMask == COLLISION_GroundPlane )
            secondsSinceCollision[COLLISION_GroundPlane] = 0;

        if ( collisionMask == COLLISION_LeftPlane )
            secondsSinceCollision[COLLISION_LeftPlane] = 0;

        if ( collisionMask == COLLISION_RightPlane )
            secondsSinceCollision[COLLISION_RightPlane] = 0;

        if ( collisionMask == COLLISION_TopPlane )
            secondsSinceCollision[COLLISION_TopPlane] = 0;

        if ( collisionMask == COLLISION_BottomPlane )
            secondsSinceCollision[COLLISION_BottomPlane] = 0;

        if ( collisionMask == COLLISION_NearPlane )
            secondsSinceCollision[COLLISION_NearPlane] = 0;

        const bool recentBoardCollision = secondsSinceCollision[COLLISION_Board] < 0.1f;
        const bool recentGroundCollision = secondsSinceCollision[COLLISION_GroundPlane] < 0.1f;
        const bool recentLeftCollision = secondsSinceCollision[COLLISION_LeftPlane] < 0.1f;
        const bool recentRightCollision = secondsSinceCollision[COLLISION_RightPlane] < 0.1f;
        const bool recentTopCollision = secondsSinceCollision[COLLISION_TopPlane] < 0.1f;
        const bool recentBottomCollision = secondsSinceCollision[COLLISION_BottomPlane] < 0.1f;
        const bool recentNearCollision = secondsSinceCollision[COLLISION_NearPlane] < 0.1f;

        // detect at rest on each collision plane and board

        const bool atRest = !stone.rigidBody.active;

        if ( detectionTimer[CONDITION_AtRestBoard].Update( dt, atRest && collisionMask == COLLISION_Board ) )
            IncrementCounter( COUNTER_AtRestBoard );

        if ( detectionTimer[CONDITION_AtRestGroundPlane].Update( dt, atRest && collisionMask == COLLISION_GroundPlane ) )
            IncrementCounter( COUNTER_AtRestGroundPlane );

        if ( detectionTimer[CONDITION_AtRestLeftPlane].Update( dt, atRest && collisionMask == COLLISION_LeftPlane ) )
            IncrementCounter( COUNTER_AtRestLeftPlane );
   
        if ( detectionTimer[CONDITION_AtRestRightPlane].Update( dt, atRest && collisionMask == COLLISION_RightPlane ) )
            IncrementCounter( COUNTER_AtRestRightPlane );

        if ( detectionTimer[CONDITION_AtRestTopPlane].Update( dt, atRest && collisionMask == COLLISION_TopPlane ) )
            IncrementCounter( COUNTER_AtRestTopPlane );

        if ( detectionTimer[CONDITION_AtRestBottomPlane].Update( dt, atRest && collisionMask == COLLISION_BottomPlane ) )
            IncrementCounter( COUNTER_AtRestBottomPlane );

        if ( detectionTimer[CONDITION_AtRestNearPlane].Update( dt, atRest && collisionMask == COLLISION_NearPlane ) )
            IncrementCounter( COUNTER_AtRestNearPlane );

        // detect sliding on each collision plane

        const float SlideTime = 0.5f;

        const bool sliding = !atRest &&
                             length( stone.rigidBody.linearVelocity ) < 10.0f &&
                             length( stone.rigidBody.angularVelocity ) < 1.0f;

        if ( detectionTimer[CONDITION_SlidingBoard].Update( dt, sliding && collisionMask == COLLISION_Board, SlideTime ) )
            IncrementCounter( COUNTER_SlidingBoard );

        if ( detectionTimer[CONDITION_SlidingGroundPlane].Update( dt, sliding && collisionMask == COLLISION_GroundPlane, SlideTime ) )
            IncrementCounter( COUNTER_SlidingGroundPlane );

        if ( detectionTimer[CONDITION_SlidingLeftPlane].Update( dt, sliding && collisionMask == COLLISION_LeftPlane, SlideTime ) )
            IncrementCounter( COUNTER_SlidingLeftPlane );

        if ( detectionTimer[CONDITION_SlidingRightPlane].Update( dt, sliding && collisionMask == COLLISION_RightPlane, SlideTime ) )
            IncrementCounter( COUNTER_SlidingRightPlane );

        if ( detectionTimer[CONDITION_SlidingTopPlane].Update( dt, sliding && collisionMask == COLLISION_TopPlane, SlideTime ) )
            IncrementCounter( COUNTER_SlidingTopPlane );

        if ( detectionTimer[CONDITION_SlidingBottomPlane].Update( dt, sliding && collisionMask == COLLISION_BottomPlane, SlideTime ) )
            IncrementCounter( COUNTER_SlidingBottomPlane );

        if ( detectionTimer[CONDITION_SlidingNearPlane].Update( dt, sliding && collisionMask == COLLISION_NearPlane, SlideTime ) )
            IncrementCounter( COUNTER_SlidingNearPlane );

        // detect spinning on each collision plane

        if ( !locked )
        {
            const float SpinTime = 0.25f;
            
            const Stone & stone = stones[0];
            
            vec3f stoneUp;
            stone.rigidBody.transform.GetUp( stoneUp );
            
            const float upSpin = fabs( dot( stone.rigidBody.angularVelocity, up ) );

            const float totalSpin = length( stone.rigidBody.angularVelocity );

            const bool spinning = length( stone.rigidBody.linearVelocity ) < 10.0 &&
                                  dot( stoneUp, up ) < 0.25f &&
                                  upSpin > 1.0f && fabs( upSpin / totalSpin ) > 0.7f &&
                                  secondsSinceLastSwipe < 1.0f;

            if ( detectionTimer[CONDITION_SpinningBoard].Update( dt, spinning && recentBoardCollision, SpinTime ) )
                IncrementCounter( COUNTER_SpinningBoard );

            if ( detectionTimer[CONDITION_SpinningGroundPlane].Update( dt, spinning && recentGroundCollision, SpinTime ) )
                IncrementCounter( COUNTER_SpinningGroundPlane );

            if ( detectionTimer[CONDITION_SpinningLeftPlane].Update( dt, spinning && recentLeftCollision, SpinTime ) )
                IncrementCounter( COUNTER_SpinningLeftPlane );

            if ( detectionTimer[CONDITION_SpinningRightPlane].Update( dt, spinning && recentRightCollision, SpinTime ) )
                IncrementCounter( COUNTER_SpinningRightPlane );

            if ( detectionTimer[CONDITION_SpinningTopPlane].Update( dt, spinning && recentTopCollision, SpinTime ) )
                IncrementCounter( COUNTER_SpinningTopPlane );
            
            if ( detectionTimer[CONDITION_SpinningBottomPlane].Update( dt, spinning && recentBottomCollision, SpinTime ) )
                IncrementCounter( COUNTER_SpinningBottomPlane );

            if ( detectionTimer[CONDITION_SpinningNearPlane].Update( dt, spinning && recentNearCollision, SpinTime ) )
                IncrementCounter( COUNTER_SpinningNearPlane );
        }

        // update orientation detection

        if ( !locked )
        {
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
        }

        // clear values for next frame

        collisionMask = 0;
        swipedThisFrame = false;

    #endif
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
        if ( ++counters[counterIndex] == 1 )
        {
            // todo: need some sort of callback or something so we can hook up to testflight
            // via Obj-C. this file needs to stay pure C++ for portability
            /*
            [TestFlight passCheckpoint:[NSString stringWithUTF8String:CounterNames[counterIndex]]];
            */
        }
    }

private:

    bool swipedThisFrame;
    double secondsSinceLastSwipe;
    uint32_t collisionMask;

    double secondsSinceCollision[COLLISION_NumValues];

    uint64_t counters[COUNTER_NumValues];
    
    DetectionTimer detectionTimer[CONDITION_NumValues];
};

#endif
