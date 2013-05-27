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
    StoneData stoneShadow;
    StoneMap stoneMap;
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

        cameraMode = 1;

        tilt = false;//true;
		locked = true;
        gravity = true;
		
        aspectRatio = 1.0f;

        lightPosition = vec3f( 10, 10, 100 );

        zoomPoint = vec3f(0,0,0);

        telemetry = NULL;

        board.Initialize( BoardSize );

        const StoneSize stoneSize = STONE_SIZE_36;

        stoneData.Initialize( stoneSize );
        stoneShadow.Initialize( stoneSize, 0.0f );

        sceneGrid.Initialize( SceneGridRes, SceneGridWidth, SceneGridHeight, SceneGridDepth );

        #if STONES
        PlaceStones();
        #endif

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
        stone.rigidBody.UpdateTransform();
        stone.rigidBody.UpdateMomentum();
        stone.UpdateVisualTransform();

        stones.push_back( stone );
        
        const uint16_t id = stone.id;
        stoneMap.insert( std::make_pair( id, stones.size() - 1 ) );

        sceneGrid.AddObject( stone.id, stone.rigidBody.position );
        
        board.SetPointState( row, column, (PointState) color );
        board.SetPointStoneId( row, column, stone.id );
        
        ValidateBoard();
        ValidateSceneGrid();
    }

    void PlaceStones()
    {
        stones.clear();
        stoneMap.clear();
        selectMap.clear();
        sceneGrid.clear();

        /*
        // shusaku ear-reddening game

        const int e = 0;
        const int w = 1;
        const int b = 2;

        int boardState[] = 
        {
            e, e, e, w, w, w, b, e, b, b, w, w, e, w, e, w, w, b, e,
            e, e, w, e, w, b, b, b, e, b, w, e, w, w, w, w, b, b, e,
            e, e, w, w, w, b, b, e, e, b, b, w, w, b, w, b, e, b, e,
            e, e, e, e, w, w, b, e, b, w, b, b, b, b, b, e, b, e, e,
            e, e, e, e, e, b, w, b, e, e, b, b, e, e, e, b, b, b, e,
            e, e, w, e, w, w, w, b, e, w, w, b, w, w, e, b, e, e, b,
            e, e, e, w, w, b, b, b, b, b, b, b, b, w, w, w, b, b, b,
            e, e, w, b, b, e, b, w, w, b, e, b, w, e, b, w, w, w, b,
            e, e, w, w, b, b, b, e, w, b, w, w, e, w, w, b, b, b, b,
            e, e, w, b, b, w, w, w, w, b, b, b, w, w, w, w, b, w, w,
            w, w, w, w, b, b, b, w, e, w, b, w, w, e, w, b, b, w, e,
            w, b, w, e, w, w, w, w, w, w, b, w, e, w, w, b, w, e, w,
            b, b, b, b, w, b, b, b, w, b, e, b, w, e, w, b, w, w, e,
            e, b, w, b, b, w, w, b, w, b, b, b, w, b, w, b, b, w, b,
            e, e, w, b, e, e, b, b, w, w, e, b, w, b, w, b, w, e, w,
            e, e, b, e, b, e, b, b, b, w, w, b, w, w, b, w, w, w, e,
            e, e, e, e, b, b, e, b, w, e, w, w, b, b, b, b, w, w, w,
            e, e, b, b, w, b, b, w, e, w, w, b, b, b, b, e, b, w, b,
            e, e, e, b, w, w, w, w, w, e, w, w, b, b, e, b, b, b, e            
        };

        for ( int i = 0; i < 19; ++i )
        {
            for ( int j = 0; j < 19; ++j )
            {
                const int row = i + 1;
                const int column = j + 1;
                const int state = boardState[(18-j)+i*19];
                if ( state == w )
                    AddStone( row, column, White );
                else if ( state == b )
                    AddStone( row, column, Black );
            }       
        }
        */

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
        
        UpdateGame( dt );

        UpdatePhysics( dt );
    }

    void UpdateCamera( float dt = 0.0f )
    {
        projectionMatrix = mat4f::perspective( 30, aspectRatio, 0.1f, 100.0f );

        if ( cameraMode == 0 )
        {
            cameraMatrix = mat4f::lookAt( vec3f( 0, 0, 45 ),
                                          vec3f( 0, 0, 0 ),
                                          vec3f( 0, -1, 0 ) );
        }
        else if ( cameraMode == 1 )
        {
            cameraMatrix = mat4f::lookAt( vec3f( 0, 25, 30 ),
                                          vec3f( 0, 2, board.GetThickness() ),
                                          vec3f( 0, 0, 1 ) );
        }
        else if ( cameraMode == 2 )
        {
            cameraMatrix = mat4f::lookAt( zoomPoint + vec3f( 0, 15, 10 ),
                                          zoomPoint,
                                          vec3f( 0, 0, 1 ) );
        }
        else if ( cameraMode == 3 )
        {
            cameraMatrix = mat4f::lookAt( zoomPoint + vec3f( 0, 0, 15 ),
                                          zoomPoint,
                                          vec3f( 0, -1, 0 ) );
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

            StoneInstance * stone = FindStoneInstance( select.stoneId, stones, stoneMap );
            if ( stone )
            {
                vec3f previousPosition = stone->rigidBody.position;

                // first snap the selected stone to the offset from
                // last select intersection point

                stone->rigidBody.position = select.intersectionPoint + select.offset;

                select.time += dt;

                const float z = SelectHeight;

                vec3f newPosition = vec3f( stone->rigidBody.position.x(),
                                           stone->rigidBody.position.y(),
                                           z );

                stone->visualOffset = stone->rigidBody.position + stone->visualOffset - newPosition;
                stone->rigidBody.position = newPosition;

                vec3f rayStart, rayDirection;
                GetPickRay( inverseClipMatrix, select.touch.point.x(), select.touch.point.y(), rayStart, rayDirection );

                float t;
                if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), select.depth, t ) )
                {
                    select.intersectionPoint = rayStart + rayDirection * t;
                    select.offset = stone->rigidBody.position - select.intersectionPoint;
                }

                // commit the result back to the scene grid
                
                sceneGrid.MoveObject( stone->id, previousPosition, stone->rigidBody.position );
                
                stone->rigidBody.UpdateTransform();

                stone->rigidBody.Activate();

                ValidateSceneGrid();
            }
        }
    }

    void UpdatePhysics( float dt )
    {
        #if PHYSICS

        ValidateSceneGrid();

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

                    float jerkZ = min( jerkImpulse.z() * 10, 10 );
                    if ( jerkZ < 2.0 )
                        jerkImpulse = vec3f( -jerkImpulseXY.y(), jerkImpulseXY.x(), 0 );
                    else
                    {
                        if ( stone.rigidBody.linearVelocity.z() <= 1.0 )
                            jerkImpulse = vec3f( 0, 0, jerkZ );
                        else
                            jerkImpulse = vec3f( 0,0,0 );
                    }
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
        params.ceiling = 25.0f;
        params.gravity = gravity ? ( 10 * 9.8f * ( tilt ? accelerometer->GetDown() : vec3f(0,0,-1) ) )
                                 : vec3f(0,0,0);

        ::UpdatePhysics( params, board, stoneData, sceneGrid, stones, stoneMap, *telemetry, frustum );

        ValidateSceneGrid();

        // update visual transform per-stone (smoothing)

        for ( int i = 0; i < stones.size(); ++i )
            stones[i].UpdateVisualTransform();

        #endif
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
                    stoneMap.erase( stone.id );
                    ValidateSceneGrid();
                }
                else
                    ++itor;
            }
        }
    }

    void ValidateBoard()
    {
        #if VALIDATION

        const int size = board.GetSize();

        // verify that stone constraints match the board state at the point

        for ( int i = 0; i < stones.size(); ++i )
        {
            StoneInstance & stone = stones[i];

            if ( stone.constrained )
            {
                const int row = stone.constraintRow;
                const int column = stone.constraintColumn;

                assert( row >= 1 );
                assert( row <= size );
                
                assert( column >= 1 );
                assert( column <= size );
                
                assert( board.GetPointState( row, column ) == ( stone.white ? White : Black ) );
                assert( board.GetPointStoneId( row, column ) == stone.id );                
            }
        }

        // iterate across all board points, verify they match the stone state

        for ( int row = 1; row <= size; ++row )
        {
            for ( int column = 1; column <= size; ++column )
            {
                int pointState = board.GetPointState( row, column );
                if ( pointState != Empty )
                {
                    uint16_t id = board.GetPointStoneId( row, column );
                    StoneInstance * stone = FindStoneInstance( id, stones );
                    assert( stone );
                    assert( stone->id == id );
                    assert( stone->white == ( pointState == White ) );
                    assert( stone->constrained );
                }
            }
        }

        #endif
    }

    void ValidateSceneGrid()
    {
        #if VALIDATION

        for ( int i = 0; i < stones.size(); ++i )
        {
            StoneInstance & stone = stones[i];

            int ix,iy,iz;
            sceneGrid.GetCellCoordinates( stone.rigidBody.position, ix, iy, iz );

            int index = sceneGrid.GetCellIndex( ix, iy, iz );

            const Cell & cell = sceneGrid.GetCell( index );

            assert( std::find( cell.objects.begin(), cell.objects.end(), stone.id ) != cell.objects.end() );
        }

        #endif
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
        cameraMode++;
        cameraMode = cameraMode % 4;

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
                if ( stone && !stone->selected )
                {
                    stone->rigidBody.linearMomentum = vec3f(0,0,0);
                    stone->rigidBody.ApplyImpulseAtWorldPoint( intersectionPoint, SelectImpulse * rayDirection );
                    stone->selected = 1;

                    SelectData select;
                    select.touch = touch;
                    select.stoneId = stone->id;
                    select.depth = stone->rigidBody.position.z();
                    select.impulse = TouchImpulse * rayDirection;
                    select.lastMoveDelta = vec3f(0,0,0);
                    select.moved = false;
                    select.constrained = stone->constrained;
                    select.constraintRow = stone->constraintRow;
                    select.constraintColumn = stone->constraintColumn;
                    select.initialPosition = stone->rigidBody.position;
                    select.initialTimestamp = touch.timestamp;
                    select.time = 0.0f;

                    // IMPORTANT: determine offset of stone position from intersection
                    // between screen ray and plane at stone z with normal (0,0,1)
                    {
                        vec3f rayStart, rayDirection;
                        GetPickRay( inverseClipMatrix, touch.point.x(), touch.point.y(), rayStart, rayDirection );
                        float t;
                        if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), select.depth, t ) )
                        {
                            select.intersectionPoint = rayStart + rayDirection * t;
                            select.offset = stone->rigidBody.position - select.intersectionPoint;
                        }
                        else
                        {
                            assert( false );        // what the fuck?
                        }
                    } 

                    selectMap.insert( std::make_pair( touch.handle, select ) );

                    if ( stone->constrained )
                    {
                        ValidateBoard();
                        stone->constrained = 0;
                        board.SetPointState( stone->constraintRow, stone->constraintColumn, Empty );
                        board.SetPointStoneId( stone->constraintRow, stone->constraintColumn, 0 );
                        ValidateBoard();
                    }
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
                select.touch = touch;
                select.moved = true;
                StoneInstance * stone = FindStoneInstance( select.stoneId, stones, stoneMap );
                if ( stone )
                {
                    vec3f rayStart, rayDirection;
                    GetPickRay( inverseClipMatrix, touch.point.x(), touch.point.y(), rayStart, rayDirection );
                    float t;
                    if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), select.depth, t ) )
                    {
                        vec3f previous = select.intersectionPoint;
                        select.intersectionPoint = rayStart + rayDirection * t;
                        select.lastMoveDelta = vec3f( select.intersectionPoint.x() - previous.x(),
                                                      select.intersectionPoint.y() - previous.y(), 0 );
                    }
                    else
                    {
                        assert( false );    // what the fuck?
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
                StoneInstance * stone = FindStoneInstance( select.stoneId, stones, stoneMap );
                if ( stone )
                {
                    stone->selected = 0;

                    // find the nearest empty point to move the stone to
                    // be extra nice to the player by searching adjacent cells
                    // and finding the one that is nearest to the current stone
                    // select position.

                    int row, column;
                    if ( board.FindNearestEmptyPoint( stone->rigidBody.position, row, column ) )
                    {
                        ValidateBoard();
                        stone->constrained = 1;
                        stone->constraintRow = row;
                        stone->constraintColumn = column;
                        stone->constraintPosition = board.GetPointPosition( row, column );
                        board.SetPointState( row, column, stone->white ? White : Black );
                        board.SetPointStoneId( row, column, stone->id );
                        stone->rigidBody.linearMomentum = vec3f(0,0,-DropMomentum);
                        stone->rigidBody.UpdateMomentum();
                        ValidateBoard();

                        vec3f previousPosition = stone->rigidBody.position;
                        vec3f newPosition = previousPosition;
                        ConstrainPosition( newPosition, stone->constraintPosition );
                        stone->visualOffset = stone->rigidBody.position + stone->visualOffset - newPosition;
                        stone->rigidBody.position = newPosition;

                        sceneGrid.MoveObject( stone->id, previousPosition, newPosition );
                    }
                    else
                    {
                        // if there is no empty point, but the stone is over the board
                        // this means the player tried to move the stone but fucked up. 
                        // be nice and revert to original stone position if it's still
                        // empty, otherwise delete the stone.

                        if ( board.FindNearestPoint( stone->rigidBody.position, row, column ) )           // hack: this is really "is the stone above the board" check
                        {
                            if ( select.constrained )
                            {
                                if ( board.GetPointState( select.constraintRow, select.constraintColumn ) == Empty )
                                {
                                    // warp back to point on board the stone was
                                    // originally dragged from and constrain

                                    row = select.constraintRow;
                                    column = select.constraintColumn;

                                    ValidateBoard();
                                    stone->constrained = 1;
                                    stone->constraintRow = row;
                                    stone->constraintColumn = column;
                                    stone->constraintPosition = board.GetPointPosition( row, column );
                                    board.SetPointState( row, column, stone->white ? White : Black );
                                    board.SetPointStoneId( row, column, stone->id );
                                    stone->rigidBody.linearMomentum = vec3f(0,0,-DropMomentum);
                                    stone->rigidBody.UpdateMomentum();
                                    ValidateBoard();

                                    vec3f previousPosition = stone->rigidBody.position;
                                    vec3f newPosition = select.initialPosition;
                                    stone->visualOffset = stone->rigidBody.position + stone->visualOffset - newPosition;
                                    stone->rigidBody.position = newPosition;

                                    sceneGrid.MoveObject( stone->id, previousPosition, newPosition );
                                }
                                else
                                {
                                    // no choice but to delete the stone
                                    stone->deleteTimer = DeleteTime;
                                }
                            }
                            else
                            {
                                // warp back to original, unconstrained position off the board
                                vec3f previousPosition = stone->rigidBody.position;
                                vec3f newPosition = select.initialPosition;
                                stone->visualOffset = stone->rigidBody.position + stone->visualOffset - newPosition;
                                stone->rigidBody.position = newPosition;
                                sceneGrid.MoveObject( stone->id, previousPosition, newPosition );
                            }
                        }
                    }

                    if ( select.moved )
                    {
                        if ( !stone->constrained )
                        {
                            // flick stone according to last touch delta position 
                            // along select plane (world space xy only)

                            if ( length_squared( select.lastMoveDelta ) > 0.1f * 0.1f )
                            {
                                const float dt = touch.timestamp - select.touch.timestamp;
                                stone->rigidBody.linearMomentum = stone->rigidBody.mass * select.lastMoveDelta / max( 1.0f / 60.0f, dt );
                                stone->rigidBody.Activate();
                            }
                        }
                    }
                    else
                    {
                        if ( touch.timestamp - select.touch.timestamp < 0.2f )
                            stone->rigidBody.ApplyImpulseAtWorldPoint( select.intersectionPoint, select.impulse );
                    }
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
                StoneInstance * stone = FindStoneInstance( select.stoneId, stones, stoneMap );
                if ( stone )
                {
                    // whoops touch was cancelled. attempt to revert the stone
                    // back to the original location before drag, if this is not
                    // possible then just delete it.

                    stone->selected = 0;

                    int row, column;

                    if ( board.FindNearestPoint( stone->rigidBody.position, row, column ) )     // hack: this is really an "stone is above board" check
                    {
                        if ( select.constrained && board.GetPointState( select.constraintRow, select.constraintColumn ) == Empty )
                        {
                            row = select.constraintRow;
                            column = select.constraintColumn;

                            ValidateBoard();
                            stone->constrained = 1;
                            stone->constraintRow = row;
                            stone->constraintColumn = column;
                            stone->constraintPosition = board.GetPointPosition( row, column );
                            board.SetPointState( row, column, stone->white ? White : Black );
                            board.SetPointStoneId( row, column, stone->id );
                            stone->rigidBody.linearMomentum = vec3f(0,0,-DropMomentum);
                            stone->rigidBody.UpdateMomentum();
                            ValidateBoard();

                            vec3f previousPosition = stone->rigidBody.position;
                            vec3f newPosition = stone->constraintPosition + vec3f(0,0,stoneData.biconvex.GetHeight()/2);
                            stone->visualOffset = stone->rigidBody.position + stone->visualOffset - newPosition;
                            stone->rigidBody.position = newPosition;

                            sceneGrid.MoveObject( stone->id, previousPosition, newPosition );
                        }
                        else
                            stone->deleteTimer = DeleteTime;
                    }
                }
                selectMap.erase( itor );
            }
        }
    }

    void OnSwipe( const vec3f & point, const vec3f & delta )
    {
        // ...
    }

    // ----------------------------------------------------------------
    // accessors
    // ----------------------------------------------------------------

    const Biconvex & GetBiconvex() const
    {
        return stoneData.biconvex;
    }

    const Biconvex & GetShadowBiconvex() const
    {
        return stoneShadow.biconvex;
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