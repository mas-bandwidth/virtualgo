#ifndef GAME_INSTANCE_H
#define GAME_INSTANCE_H

#include "Physics.h"
#include "Board.h"
#include "StoneData.h"
#include "StoneInstance.h"
#include "Telemetry.h"
#include "Touch.h"

class GameInstance
{
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
    bool zoomed;
    bool gravity;

    float smoothZoom;
    float aspectRatio;

    uint32_t stoneId;

    Telemetry * telemetry;

    SelectMap selectMap;

public:

	GameInstance()
	{
        stoneId = 0;

		locked = true;
		zoomed = false;
        gravity = false;
		
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
        selectMap.clear();

        bool white = true;
        for ( int i = 1; i <= BoardSize; ++i )
        {
            for ( int j = 1; j <= BoardSize; ++j )
            {
                StoneInstance stone;
                stone.Initialize( stoneData, stoneId++, white );
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

        PhysicsParameters params;

        params.dt = dt;
        params.ceiling = smoothZoom * 0.5f;
        params.gravity = gravity ? ( 10 * 9.8f * ( locked ? vec3f(0,0,-1) : accelerometer.GetDown() ) )
                                 : vec3f(0,0,0);

        ::UpdatePhysics( params, board, stoneData, stones, *telemetry, frustum );
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

    StoneInstance * FindStoneInstance( uint32_t id )
    {
        for ( int i = 0; i < stones.size(); ++i )
        {
            if ( stones[i].id == id ) 
                return &stones[i];
        }
        return NULL;
    }

    void InferStoneMomentum( StoneInstance & stone, const vec3f & prevPosition, const vec3f & newPosition, float dt, float threshold = 0.1f * 0.1f )
    {
        const vec3f delta = newPosition - prevPosition;

        if ( length_squared( delta ) > threshold )
            stone.rigidBody.linearMomentum = stone.rigidBody.mass * delta / max( 1.0f / 60.0f, dt );
        
        stone.rigidBody.Activate();
    }

    // -------------------------------------------------------
    // event handling
    // -------------------------------------------------------

    void OnDoubleTap( const vec3f & point )
    {
        if ( locked )
        {
            PlaceStones();
        }
        else
        {
            if ( zoomed )
                telemetry->IncrementCounter( COUNTER_ZoomedOut );
            else
                telemetry->IncrementCounter( COUNTER_ZoomedIn );
        
            zoomed = !zoomed;
        }
    }

    StoneInstance * PickStone( const vec3f & rayStart, const vec3f & rayDirection,
                               float & pick_t, vec3f & pick_point, vec3f & pick_normal )
    {
        pick_t = FLT_MAX;
        StoneInstance * pick_stone = NULL;

        for ( int i = 0; i < stones.size(); ++i )
        {
            StoneInstance & stone = stones[i];

            float t;
            vec3f intersectionPoint, intersectionNormal;            
            if ( IntersectRayStone( stoneData.biconvex, stone.rigidBody.transform, rayStart, rayDirection, t, intersectionPoint, intersectionNormal, FatFingerBonus ) )
            {
                if ( t < pick_t )
                {
                    pick_stone = &stone;
                    pick_t = t;
                    pick_point = intersectionPoint;
                    pick_normal = intersectionNormal;
                }
            }
        }
        
        return pick_stone;
    }

    void OnTouchesBegan( Touch * touches, int numTouches )
    {
        for ( int i = 0; i < numTouches; ++i )
        {
            Touch & touch = touches[i];

            if ( selectMap.find( touch.handle ) == selectMap.end() )
            {
                vec3f rayStart, rayDirection;
                GetPickRay( inverseClipMatrix, touch.point.x(), touch.point.y(), rayStart, rayDirection );

                float t;
                vec3f intersectionPoint, intersectionNormal;
                StoneInstance * stone = PickStone( rayStart, rayDirection, t, intersectionPoint, intersectionNormal );
                if ( stone )
                {
                    stone->rigidBody.ApplyImpulseAtWorldPoint( intersectionPoint, TouchImpulse * rayDirection );
                    stone->selected = 1;

                    SelectData select;
                    select.stoneId = stone->id;
                    select.touchHandle = touch.handle;
                    select.point = intersectionPoint;
                    select.depth = intersectionPoint.z();
                    select.offset = stone->rigidBody.position - intersectionPoint;
                    select.intersectionPoint = intersectionPoint;
                    select.prevIntersectionPoint = intersectionPoint;
                    select.timestamp = touch.timestamp;

                    selectMap.insert( std::make_pair( touch.handle, select ) );
                }
            }
        }
    }

    void OnTouchesMoved( Touch * touches, int numTouches )
    {
        for ( int i = 0; i < numTouches; ++i )
        {
            Touch & touch = touches[i];
            SelectMap::iterator itor = selectMap.find( touch.handle );
            if ( itor != selectMap.end() ) 
            {
                SelectData & select = itor->second;
                StoneInstance * stone = FindStoneInstance( select.stoneId );
                if ( stone )
                {
                    vec3f rayStart, rayDirection;
                    GetPickRay( inverseClipMatrix, touch.point.x(), touch.point.y(), rayStart, rayDirection );
        
                    // IMPORTANT: first update intersect depth. this way stones can lift
                    // from the ground to the board and then not snap back to ground when dragged
                    // off the board (looks bad)

                    float t;
                    vec3f intersectionPoint, intersectionNormal;
                    if ( IntersectRayStone( stoneData.biconvex, stone->rigidBody.transform, rayStart, rayDirection, t, intersectionPoint, intersectionNormal, FatFingerBonus ) )
                    {
                        if ( intersectionPoint.z() > select.depth )
                        {
                            select.depth = intersectionPoint.z();
                            select.offset = stone->rigidBody.position - intersectionPoint;
                        }
                    }

                    // next, intersect with the plane at select depth and move the stone
                    // such that it is offset from the intersection point with this plane

                    if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), select.depth, t ) )
                    {
                        select.prevIntersectionPoint = select.intersectionPoint;
                        select.intersectionPoint = rayStart + rayDirection * t;
                        select.timestamp = touch.timestamp;

                        stone->rigidBody.position = select.intersectionPoint + select.offset;
                    }
                }
                else
                {
                    selectMap.erase( itor );
                }
            }
        }
    }

    void OnTouchesEnded( Touch * touches, int numTouches )
    {
        for ( int i = 0; i < numTouches; ++i )
        {
            Touch & touch = touches[i];
            SelectMap::iterator itor = selectMap.find( touch.handle );
            if ( itor != selectMap.end() )
            {
                SelectData & select = itor->second;
                StoneInstance * stone = FindStoneInstance( select.stoneId );
                if ( stone )
                {
                    stone->selected = 0;
                    
                    InferStoneMomentum( *stone,
                                        select.prevIntersectionPoint, 
                                        select.intersectionPoint, 
                                        touch.timestamp - select.timestamp );
                }
                selectMap.erase( itor );
            }
        }
    }

    void OnTouchesCancelled( Touch * touches, int numTouches )
    {
        for ( int i = 0; i < numTouches; ++i )
        {
            Touch & touch = touches[i];
            SelectMap::iterator itor = selectMap.find( touch.handle );
            if ( itor != selectMap.end() )
            {
                SelectData & select = itor->second;
                StoneInstance * stone = FindStoneInstance( select.stoneId );
                if ( stone )
                {
                    stone->selected = 0;
                    
                    InferStoneMomentum( *stone,
                                       select.prevIntersectionPoint,
                                       select.intersectionPoint,
                                       touch.timestamp - select.timestamp );
                }
                selectMap.erase( itor );
            }
        }
    }

    // ----------------------------------------------------------------
    // accessors
    // ----------------------------------------------------------------

    const Biconvex & GetBiconvex() const
    {
        return stoneData.biconvex;
    }

    const Board & GetBoard() const
    {
        return board;
    }

    const std::vector<StoneInstance> & GetStones() const
    {
        return stones;
    }

    const mat4f & GetCameraMatrix() const
    {
        return cameraMatrix;
    }

    const mat4f & GetProjectionMatrix() const
    {
        return projectionMatrix;
    }

    const mat4f & GetClipMatrix() const
    {
        return clipMatrix;
    }

    const mat3f & GetNormalMatrix() const
    {
        return normalMatrix;
    }

    const vec3f & GetLightPosition() const
    {
        return lightPosition;
    }

    bool IsLocked() const
    {
        return locked;
    }
};

#endif