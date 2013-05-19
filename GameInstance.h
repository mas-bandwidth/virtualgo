#ifndef GAME_INSTANCE_H
#define GAME_INSTANCE_H

#include "Physics.h"
#include "Board.h"
#include "StoneData.h"
#include "StoneInstance.h"
#include "SceneGrid.h"
#include "Telemetry.h"
#include "Touch.h"
#include <algorithm>
#include <functional>

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

    int cameraMode;

    bool locked;
    bool gravity;
    bool tilt;

    float smoothZoom;
    float aspectRatio;

    uint32_t stoneId : 16;

    Telemetry * telemetry;
    Accelerometer * accelerometer;

    SelectMap selectMap;

    SceneGrid sceneGrid;

    vec3f zoomPoint;

public:

	GameInstance()
	{
        stoneId = 0;

        cameraMode = 0;

        tilt = false;//true;
		locked = true;
        gravity = true;
		
//        smoothZoom = GetTargetZoom();
        
        aspectRatio = 1.0f;

        lightPosition = vec3f( 10, 10, 50 );

        zoomPoint = vec3f(0,0,0);

        telemetry = NULL;

        board.Initialize( BoardSize );

        stoneData.Initialize( STONE_SIZE_36 );

        sceneGrid.Initialize( SceneGridRes, SceneGridWidth, SceneGridHeight, SceneGridDepth );

        PlaceStones();

        UpdateCamera();
	}

    void Initialize( Telemetry & telemetry, Accelerometer & accelerometer, float aspectRatio )
    {
        this->telemetry = &telemetry;
        this->accelerometer = &accelerometer;
        this->aspectRatio = aspectRatio;
    }

    void SetAspectRatio( float aspectRatio )
    {
        this->aspectRatio = aspectRatio;
    }

    void AddStone( int row, int column, int color, bool constrained = true )
    {
        vec3f variance = vec3f( random_float( - PlacementVariance, + PlacementVariance ),
                                random_float( - PlacementVariance, + PlacementVariance ),
                                0 );

        StoneInstance stone;
        stone.Initialize( stoneData, stoneId++, color == White );
        stone.rigidBody.position = board.GetPointPosition( row, column ) + vec3f( 0, 0, stoneData.biconvex.GetHeight() / 2 ) + variance;
        stone.rigidBody.orientation = quat4f::axisRotation( random_float(0,2*pi), vec3f(0,0,1) );
        stone.rigidBody.linearMomentum = vec3f(0,0,0);
        stone.rigidBody.angularMomentum = vec3f(0,0,0);
        stone.rigidBody.Activate();
        stone.constrained = 1;
        stone.constraintRow = row;
        stone.constraintColumn = column;
        stone.constraintPosition = board.GetPointPosition( row, column );
        stones.push_back( stone );
        sceneGrid.AddObject( stone.id, stone.rigidBody.position );
        board.SetPointState( row, column, (PointState) color );
        board.SetPointStoneId( row, column, stone.id );
    }

    void PlaceStones()
    {
        stones.clear();
        selectMap.clear();
        sceneGrid.clear();

        // Miyamoto Naoki vs Go Seigen 9x9
        // https://www.youtube.com/watch?v=VsBqYNR5P3U

        AddStone( 1, 2, Black );
        AddStone( 1, 3, White );
        AddStone( 1, 4, White );
        AddStone( 1, 8, White );

        AddStone( 2, 2, Black );
        AddStone( 2, 3, White );
        AddStone( 2, 5, White );
        AddStone( 2, 7, White );
        AddStone( 2, 8, Black );
        AddStone( 2, 9, Black );

        AddStone( 3, 2, Black );
        AddStone( 3, 3, Black );
        AddStone( 3, 4, White );
        AddStone( 3, 5, White );
        AddStone( 3, 6, White );
        AddStone( 3, 7, Black );
        AddStone( 3, 8, Black );

        AddStone( 4, 3, Black );
        AddStone( 4, 4, White );
        AddStone( 4, 6, White );
        AddStone( 4, 7, White );
        AddStone( 4, 8, Black );

        AddStone( 5, 1, Black );
        AddStone( 5, 2, Black );
        AddStone( 5, 5, Black );
        AddStone( 5, 7, White );
        AddStone( 5, 8, Black );

        AddStone( 6, 1, Black );
        AddStone( 6, 2, White );
        AddStone( 6, 3, Black );
        AddStone( 6, 4, Black );
        AddStone( 6, 6, Black );
        AddStone( 6, 7, White );
        AddStone( 6, 8, Black );
        
        AddStone( 7, 1, Black );
        AddStone( 7, 2, White );
        AddStone( 7, 3, White );
        AddStone( 7, 4, Black );
        AddStone( 7, 6, White );
        AddStone( 7, 7, Black );

        AddStone( 8, 1, White );
        AddStone( 8, 2, White );
        AddStone( 8, 3, White );
        AddStone( 8, 4, White );
        AddStone( 8, 5, White );
        AddStone( 8, 6, White );
        AddStone( 8, 7, Black );

        AddStone( 9, 2, Black );
        AddStone( 9, 3, White );
        AddStone( 9, 4, Black );
        AddStone( 9, 6, White );
        AddStone( 9, 7, Black );

        /*
        // add stones on the star points

        int numStarPoints;
        vec3f pointPosition[MaxStarPoints];
        board.GetStarPoints( pointPosition, numStarPoints );

        bool white = false;
        for ( int i = 0; i < numStarPoints; ++i )
        {
            StoneInstance stone;
            stone.Initialize( stoneData, stoneId++, white );
            stone.rigidBody.position = pointPosition[i] + vec3f(0,0,stoneData.biconvex.GetHeight()/2);
            stone.rigidBody.orientation = quat4f(1,0,0,0);
            stone.rigidBody.linearMomentum = vec3f(0,0,0);
            stone.rigidBody.angularMomentum = vec3f(0,0,0);
            stone.rigidBody.Activate();
            stones.push_back( stone );
            sceneGrid.AddObject( stone.id, stone.rigidBody.position );
            white = !white;
        }
        */

        /*
        // add stones on every point

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

                sceneGrid.AddObject( stone.id, stone.rigidBody.position );
            }
        }
        */
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

    void Update( float dt )
    {
        UpdateCamera( dt );

        UpdateTouch( dt );
        
        UpdatePhysics( dt );

        UpdateGame( dt );
    }

    void UpdateCamera( float dt = 0.0f )
    {
//        const float targetZoom = GetTargetZoom();
//        smoothZoom += ( targetZoom - smoothZoom ) * ( zoomed ? ZoomInTightness : ZoomOutTightness );
        
        projectionMatrix = mat4f::perspective( 40, aspectRatio, 0.1f, 100.0f );

        if ( cameraMode == 0 )
        {
            cameraMatrix = mat4f::lookAt( vec3f( 0, 0, 35 ),
                                         vec3f( 0, 0, 0 ),
                                         vec3f( 0, 1, 0 ) );
        }
        else if ( cameraMode == 1 )
        {
            cameraMatrix = mat4f::lookAt( vec3f( 0, 22, 20 ),
                                          //vec3f( 0, 24, 22.5f ),
                                          vec3f( 0, 2, 0 ),
                                          vec3f( 0, 0, 1 ) );
        }
        else if ( cameraMode == 2 )
        {
            cameraMatrix = mat4f::lookAt( zoomPoint + vec3f( 0, 10, 5 ),
                                          zoomPoint,
                                          vec3f( 0, 0, 1 ) );
        }

        clipMatrix = projectionMatrix * cameraMatrix;

        InvertMatrix( clipMatrix, inverseClipMatrix );

        normalMatrix.load( cameraMatrix );
    }

    void UpdateTouch( float dt )
    {
        for ( SelectMap::iterator itor = selectMap.begin(); itor != selectMap.end(); ++itor ) 
        {
            SelectData & select = itor->second;

            StoneInstance * stone = FindStoneInstance( select.stoneId, stones );
            if ( stone )
            {
                vec3f previousPosition = stone->rigidBody.position;
                stone->rigidBody.position = select.intersectionPoint + select.offset;
                sceneGrid.MoveObject( stone->id, previousPosition, stone->rigidBody.position );
                stone->rigidBody.Activate();
            }
        }
    }

    void UpdatePhysics( float dt )
    {
        // calculate frustum planes for collision

        Frustum frustum;
        CalculateFrustumPlanes( clipMatrix, frustum );
        
        // apply jerk acceleration to stones

        vec3f jerkAcceleration = accelerometer->GetJerkAcceleration();
        const float jerk = length( jerkAcceleration );
        if ( jerk > JerkThreshold )
        {
            for ( int i = 0; i < stones.size(); ++i )
            {
                StoneInstance & stone = stones[i];

                vec3f varianceScale = vec3f( random_float( 1.0f - JerkVariance, 1.0f + JerkVariance ),
                                             random_float( 1.0f - JerkVariance, 1.0f + JerkVariance ),
                                             random_float( 1.0f - JerkVariance, 1.0f + JerkVariance ) );

                vec3f jerkImpulse = JerkScale * jerkAcceleration;

                if ( locked )
                {
                    vec3f jerkImpulseXY( jerkImpulse.x(), jerkImpulse.y(), 0 );

                    float jerkLengthXY = length( jerkImpulseXY );
                    if ( jerkLengthXY > JerkMax )
                        jerkImpulseXY = jerkImpulseXY / jerkLengthXY * JerkMax;

                    float jerkZ = 0;
                    /*
                    float jerkZ = min( jerkImpulse.z() * 10, 2.5 );
                    if ( jerkZ < 2.0 )
                        jerkZ = 0.0;
                     */

//                    jerkImpulse = vec3f( jerkImpulseXY.x(), jerkImpulseXY.y(), jerkZ );

                    // hack: switch x and y due to camera orientation
                    jerkImpulse = vec3f( -jerkImpulseXY.y(), jerkImpulseXY.x(), jerkZ );
                }
                
                stone.rigidBody.ApplyImpulse( jerkImpulse * varianceScale * stone.rigidBody.mass );
            }
        }

        // detect when the user wants to launch the stone into the air

        if ( !locked && jerk > LaunchThreshold )
        {
            for ( int i = 0; i < stones.size(); ++i )
            {
                StoneInstance & stone = stones[i];

                vec3f varianceScale = vec3f( random_float( 1.0f - LaunchVariance, 1.0f + LaunchVariance ),
                                             random_float( 1.0f - LaunchVariance, 1.0f + LaunchVariance ),
                                             random_float( 1.0f - LaunchVariance, 1.0f + LaunchVariance ) );

                vec3f jerkImpulse = jerkAcceleration * vec3f( LaunchMomentum*0.66f, LaunchMomentum*0.66f, LaunchMomentum*1.5f );

                stone.rigidBody.ApplyImpulse( jerkImpulse * varianceScale );
            }

            telemetry->IncrementCounter( COUNTER_AppliedImpulse );
        }
     
        // update physics sim

        PhysicsParameters params;

        params.dt = dt;
        params.locked = locked;
        params.ceiling = smoothZoom * 0.5f;
        params.gravity = gravity ? ( 10 * 9.8f * ( tilt ? accelerometer->GetDown() : vec3f(0,0,-1) ) )
                                 : vec3f(0,0,0);

        ::UpdatePhysics( params, board, stoneData, sceneGrid, stones, *telemetry, frustum );
    }

    void UpdateGame( float dt )
    {
        if ( locked )
        {
            // iterate across all stone instances -- if stone instance
            // is no selected or constrained, increase delete timer.
            // remove stones that have exceeded the delete timer.

            std::vector<StoneInstance>::iterator itor = stones.begin();
            while ( itor != stones.end() )
            {
                StoneInstance & stone = *itor;
                
                if ( !stone.constrained && !stone.selected )
                    stone.deleteTimer += dt;
                else
                    stone.deleteTimer = 0.0f;
                    
                if ( stone.deleteTimer > DeleteTime )
                {
                    sceneGrid.RemoveObject( stone.id, stone.rigidBody.position );
                    itor = stones.erase( itor );
                }
                else
                    ++itor;
            }
        }
    }

    /*
    float GetTargetZoom() const
    {
        return zoomed ? ZoomIn : ZoomOut;
    }
     */

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

    void InferStoneMomentum( StoneInstance & stone, const vec3f & prevPosition, const vec3f & newPosition, float dt, float threshold = 0.1f * 0.1f )
    {
        // todo: max flick momentum

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
        cameraMode++;
        cameraMode = cameraMode % 3;

        /*
        vec3f rayStart, rayDirection;
        GetPickRay( inverseClipMatrix, point.x(), point.y(), rayStart, rayDirection );
        
        float t = 0;
        if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), board.GetThickness(), t ) )
        {
            vec3f intersectionPoint = rayStart + rayDirection * t;
            
            float x = intersectionPoint.x();
            float y = intersectionPoint.y();
            
            float bx, by;
            board.GetBounds( bx, by );
            
            x = clamp( x, -bx, bx );
            y = clamp( y, -by, by );

            zoomPoint = vec3f( x, y, board.GetThickness() );
        }
        */
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
                    stone->rigidBody.linearMomentum = vec3f(0,0,0);
                    stone->rigidBody.ApplyImpulseAtWorldPoint( intersectionPoint, TouchImpulse * rayDirection );
                    stone->selected = 1;

                    if ( stone->constrained )
                    {
                        stone->constrained = 0;
                        board.SetPointState( stone->constraintRow, stone->constraintColumn, Empty );
                    }

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
                StoneInstance * stone = FindStoneInstance( select.stoneId, stones );
                if ( stone )
                {
                    vec3f rayStart, rayDirection;
                    GetPickRay( inverseClipMatrix, touch.point.x(), touch.point.y(), rayStart, rayDirection );
        
                    // intersect with the plane at select depth and move the stone
                    // such that it is offset from the intersection point with this plane.
                    // then, push the go stone above the go board.

                    float t;
                    if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), select.depth, t ) )
                    {
                        select.prevIntersectionPoint = select.intersectionPoint;
                        select.intersectionPoint = rayStart + rayDirection * t;
                        select.timestamp = touch.timestamp;
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
                StoneInstance * stone = FindStoneInstance( select.stoneId, stones );
                if ( stone )
                {
                    stone->selected = 0;

                    int row, column;
                    if ( board.FindNearestPoint( stone->rigidBody.position, row, column ) )
                    {
                        if ( board.GetPointState( row, column ) == Empty )
                        {
                            stone->constrained = 1;
                            stone->constraintRow = row;
                            stone->constraintColumn = column;
                            stone->constraintPosition = board.GetPointPosition( row, column );
                        }
                        else
                        {
                            stone->deleteTimer = DeleteTime;
                        }
                    }

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
                StoneInstance * stone = FindStoneInstance( select.stoneId, stones );
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

    void OnSwipe( const vec3f & point, const vec3f & delta )
    {
        const vec3f up = -normalize( accelerometer->GetSmoothedAcceleration() );

        for ( int i = 0; i < stones.size(); ++i )
        {
            StoneInstance & stone = stones[i];
            stone.rigidBody.angularMomentum += SwipeMomentum * up;
            stone.rigidBody.Activate();
        }

        telemetry->IncrementCounter( COUNTER_Swiped );
        telemetry->SetSwipedThisFrame();
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