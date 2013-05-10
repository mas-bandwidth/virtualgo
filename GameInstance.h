#ifndef GAME_INSTANCE_H
#define GAME_INSTANCE_H

#include "Physics.h"
#include "Board.h"
#include "StoneData.h"
#include "StoneInstance.h"
#include "Telemetry.h"

class GameInstance
{
public:

    // todo: this should become private. GameInstance is in charge!

    Board board;

    StoneData stoneData;

    std::vector<StoneInstance> stones;

    vec3f lightPosition;

    mat4f projectionMatrix;
    mat4f cameraMatrix;
    mat3f normalMatrix;
    mat4f clipMatrix;
    mat4f inverseClipMatrix;

    bool locked;
    bool paused;
    bool zoomed;

    float smoothZoom;
    float aspectRatio;

    Telemetry * telemetry;

	GameInstance()
	{
        #if LOCKED
		locked = true;
        #else
        locked = false;
        #endif

		paused = false;
		zoomed = false;
		
        smoothZoom = GetTargetZoom();
        aspectRatio = 1.0f;

        lightPosition = vec3f( 30, 30, 100 );

        telemetry = NULL;

        board.Initialize( BoardSize );

        stoneData.Initialize( STONE_SIZE_32 );

        PlaceStones();

        UpdateCamera();
	}

    void Initialize( Telemetry & telemetry, float aspectRatio )
    {
        this->telemetry = &telemetry;
        this->aspectRatio = aspectRatio;
    }

    void PlaceStones()
    {
        stones.clear();

        bool white = true;
        for ( int i = 1; i <= BoardSize; ++i )
        {
            for ( int j = 1; j <= BoardSize; ++j )
            {
                StoneInstance stone;
                stone.Initialize( stoneData, white );
                stone.rigidBody.position = board.GetPointPosition( i, j ) + vec3f( 0, 0, stoneData.biconvex.GetHeight() / 2 );
                stone.rigidBody.orientation = quat4f(1,0,0,0);
                stone.rigidBody.linearMomentum = vec3f(0,0,0);
                stone.rigidBody.angularMomentum = vec3f(0,0,0);
                stone.rigidBody.Activate();
                stones.push_back( stone );
                //white = !white;
            }
        }
    }

    void InvertMatrix( const mat4f & matrix, mat4f & inverse )
    {
        // todo: this needs to be made general without relying on GLKMatrix
        float data[16];
        matrix.store( data );
        GLKMatrix4 glkMatrix = GLKMatrix4MakeWithArray( data );
        bool invertible;
        GLKMatrix4 glkMatrixInverse = GLKMatrix4Invert( glkMatrix, &invertible );
        assert( invertible );
        inverse.load( glkMatrixInverse.m );
    }

    void UpdateCamera( float dt = 0.0f )
    {
        const float targetZoom = GetTargetZoom();
        
        smoothZoom += ( targetZoom - smoothZoom ) * ( zoomed ? ZoomInTightness : ZoomOutTightness );
        
        projectionMatrix = mat4f::perspective( 40, aspectRatio, 0.1f, 100.0f );

        cameraMatrix = mat4f::lookAt( vec3f( 0, 0, smoothZoom ),
                                      vec3f( 0, 0, 0 ),
                                      vec3f( 0, 1, 0 ) );

        clipMatrix = projectionMatrix * cameraMatrix;

        InvertMatrix( clipMatrix, inverseClipMatrix );

        normalMatrix.load( cameraMatrix );
    }

    void UpdatePhysics( float dt, const Accelerometer & accelerometer )
    {
        // calculate frustum planes for collision

        Frustum frustum;
        CalculateFrustumPlanes( clipMatrix, frustum );
        
        // todo: perhaps jerk and launch are best combined into some non-linear scale of high pass accelerometer?

        // apply jerk acceleration to stones

        const vec3f & jerkAcceleration = accelerometer.GetJerkAcceleration();
        const float jerk = length( jerkAcceleration );
        if ( jerk > JerkThreshold )
        {
            for ( int i = 0; i < stones.size(); ++i )
            {
                StoneInstance & stone = stones[i];
                stone.rigidBody.ApplyImpulse( JerkScale * jerkAcceleration * stone.rigidBody.mass );
            }
        }

        // detect when the user wants to launch the stone into the air

        if ( jerk > LaunchThreshold )
        {
            for ( int i = 0; i < stones.size(); ++i )
            {
                StoneInstance & stone = stones[i];
                stone.rigidBody.ApplyImpulse( jerkAcceleration * vec3f( LaunchMomentum*0.66f, LaunchMomentum*0.66f, LaunchMomentum ) );
            }

            telemetry->IncrementCounter( COUNTER_AppliedImpulse );
        }
     
        // update physics sim

        vec3f gravity = 10 * 9.8f * ( locked ? vec3f(0,0,-1) : accelerometer.GetDown() );
        
        ::UpdatePhysics( dt, board, stoneData, stones, *telemetry,
                         frustum, gravity, smoothZoom * 0.5f );
    }

    float GetTargetZoom() const
    {
        return zoomed ? ZoomIn : ZoomOut;
    }

    bool IsScreenPointOnBoard( const vec3f & point )
    {
        vec3f rayStart, rayDirection;
        GetPickRay( inverseClipMatrix, point.x(), point.y(), rayStart, rayDirection );        

        float t = 0;
        if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), board.GetThickness(), t ) )
        {
            vec3f intersectionPoint = rayStart + rayDirection * t;

            const float x = intersectionPoint.x();
            const float y = intersectionPoint.y();

            float bx, by;
            board.GetBounds( bx, by );

            return x >= -bx && x <= bx && y >= -by && y <= by;
        }

        return false;
    }

    // -------------------------------------------------------
    // event handling
    // -------------------------------------------------------

    void OnDoubleTap( const vec3f & point )
    {
        #if LOCKED

            PlaceStones();

        #else

            if ( zoomed )
                telemetry->IncrementCounter( COUNTER_ZoomedOut );
            else
                telemetry->IncrementCounter( COUNTER_ZoomedIn );
        
            game.zoomed = !game.zoomed;

        #endif
    }
};

#endif