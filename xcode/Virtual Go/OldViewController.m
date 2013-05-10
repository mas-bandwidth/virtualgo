//
//  ViewController.m
//  Virtual Go
//
//  Created by Glenn Fiedler on 4/13/13.
//  Copyright (c) 2013 Glenn Fiedler. All rights reserved.
//

#import "ViewController.h"
#import "testflight/TestFlight.h"
#import "OpenGL.h"
#import "Config.h"
#import "Common.h"
#import "Render.h"
#import "Physics.h"
#import "Telemetry.h"
#import "Accelerometer.h"
#import "MeshGenerators.h"
#import "GameInstance.h"

bool iPad()
{
    return UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad;
}

void HandleCounterNotify( int counterIndex, uint64_t counterValue, const char * counterName )
{
    if ( counterValue == 1 )
        [TestFlight passCheckpoint:[NSString stringWithUTF8String:counterName]];
}

@interface ViewController ()
{    
    // converted

    OpenGL * opengl;

    GameInstance game;

    Telemetry telemetry;

    Accelerometer accelerometer;

    // to be cleaned up below this line

    Mesh<Vertex> _stoneMesh;
    GLuint _stoneProgramWhite;
    GLuint _stoneProgramBlack;
    GLuint _stoneVertexBuffer;
    GLuint _stoneIndexBuffer;
    GLint _stoneUniformsBlack[NUM_UNIFORMS];
    GLint _stoneUniformsWhite[NUM_UNIFORMS];
    
    Mesh<TexturedVertex> _boardMesh;
    GLuint _boardProgram;
    GLuint _boardTexture;
    GLuint _boardVertexBuffer;
    GLuint _boardIndexBuffer;
    GLint _boardUniforms[NUM_UNIFORMS];

    GLuint _shadowProgram;
    GLint _shadowUniforms[NUM_UNIFORMS];

    Mesh<TexturedVertex> _gridMesh;
    GLuint _gridProgram;
    GLuint _gridVertexBuffer;
    GLuint _gridIndexBuffer;
    GLint _gridUniforms[NUM_UNIFORMS];
    GLuint _lineTexture;

    Mesh<TexturedVertex> _pointMesh;
    GLuint _pointProgram;
    GLuint _pointVertexBuffer;
    GLuint _pointIndexBuffer;
    GLint _pointUniforms[NUM_UNIFORMS];
    GLuint _pointTexture;

    Mesh<TexturedVertex> _floorMesh;
    GLuint _floorProgram;
    GLuint _floorTexture;
    GLuint _floorVertexBuffer;
    GLuint _floorIndexBuffer;
    GLint _floorUniforms[NUM_UNIFORMS];

    bool _swipeStarted;
    UITouch * _swipeTouch;
    float _swipeTime;
    CGPoint _swipeStartPoint;

    bool _holdStarted;
    UITouch * _holdTouch;
    float _holdTime;
    CGPoint _holdPoint;

    UITouch * _firstPlacementTouch;
    double _firstPlacementTimestamp;
    
    bool _selected;
    UITouch * _selectTouch;
    CGPoint _selectPoint;
    float _selectDepth;
    vec3f _selectOffset;
    double _selectTimestamp;
    double _selectPrevTimestamp;
    vec3f _selectIntersectionPoint;
    vec3f _selectPrevIntersectionPoint;
}

@property (strong, nonatomic) EAGLContext *context;

- (void)setupGL;
- (void)tearDownGL;

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.view.multipleTouchEnabled = YES;

    telemetry.SetCounterNotifyFunc( HandleCounterNotify );

    const float aspectRatio = fabsf( self.view.bounds.size.width / self.view.bounds.size.height );

    game.Initialize( telemetry, aspectRatio );

    GenerateBiconvexMesh( _stoneMesh, game.stoneData.biconvex, 3 );
    GenerateFloorMesh( _floorMesh );
    GenerateBoardMesh( _boardMesh, game.board );
    GenerateGridMesh( _gridMesh, game.board );
    GenerateStarPointsMesh( _pointMesh, game.board );

    UIAccelerometer * uiAccelerometer = [UIAccelerometer sharedAccelerometer];
    uiAccelerometer.updateInterval = 1 / AccelerometerFrequency;
    uiAccelerometer.delegate = self;
    
    self.context = [ [EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 ];

    if (!self.context)
    {
        NSLog( @"Failed to create ES context" );
    }
    
    GLKView * view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat16;
        
    opengl = [[OpenGL alloc] init];
   
    [self setupGL];

    // ----------------------------------------------------
    // todo: move this setup into the game instance
    
    _swipeStarted = false;
    _holdStarted = false;
    _selected = false;

    _firstPlacementTouch = nil;

    // ----------------------------------------------------
}

- (void)dealloc
{
    [self tearDownGL];

    if ( [EAGLContext currentContext] == self.context )
    {
        [EAGLContext setCurrentContext:nil];
    }

    [ [NSNotificationCenter defaultCenter] removeObserver:self ];
}

- (void)didReceiveMemoryWarning
{
//    NSLog( @"did receive memory warning" );

    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil))
    {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context)
        {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
        
    self.preferredFramesPerSecond = 60;

    glEnable( GL_DEPTH_TEST );

    glDepthFunc( GL_LESS );

    glEnable( GL_CULL_FACE );

    // stone shader, textures, VBs/IBs etc.
    {    
        _stoneProgramBlack = [opengl loadShader:@"BlackStoneShader"];
        _stoneProgramWhite = [opengl loadShader:@"WhiteStoneShader"];

        [opengl generateVBAndIBFromMesh:_stoneMesh vertexBuffer:_stoneVertexBuffer indexBuffer:_stoneIndexBuffer];

        _stoneUniformsBlack[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _stoneProgramBlack, "normalMatrix" );
        _stoneUniformsBlack[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _stoneProgramBlack, "modelViewProjectionMatrix" );
        _stoneUniformsBlack[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _stoneProgramBlack, "lightPosition" );

        _stoneUniformsWhite[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _stoneProgramWhite, "normalMatrix" );
        _stoneUniformsWhite[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _stoneProgramWhite, "modelViewProjectionMatrix" );
        _stoneUniformsWhite[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _stoneProgramWhite, "lightPosition" );
    }
    
    // board shader, textures, VBs/IBs etc.
    {
        _boardProgram = [opengl loadShader:@"BoardShader"];

        [opengl generateVBAndIBFromTexturedMesh:_boardMesh vertexBuffer:_boardVertexBuffer indexBuffer:_boardIndexBuffer];
        
        _boardTexture = [opengl loadTexture:@"wood.jpg"];

        _boardUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _boardProgram, "normalMatrix" );
        _boardUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _boardProgram, "modelViewProjectionMatrix" );
        _boardUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _boardProgram, "lightPosition" );
    }

    // shadow shader, uniforms etc.
    {
        _shadowProgram = [opengl loadShader:@"ShadowShader"];

        _shadowUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _shadowProgram, "normalMatrix" );
        _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _shadowProgram, "modelViewProjectionMatrix" );
        _shadowUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _shadowProgram, "lightPosition" );
        _shadowUniforms[UNIFORM_ALPHA] = glGetUniformLocation( _shadowProgram, "alpha" );
    }
  
    // grid shader, textures, VBs/IBs etc.
    {
        _gridProgram = [opengl loadShader:@"GridShader"];

        [opengl generateVBAndIBFromTexturedMesh:_gridMesh vertexBuffer:_gridVertexBuffer indexBuffer:_gridIndexBuffer];
        
        _gridUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _gridProgram, "normalMatrix" );
        _gridUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _gridProgram, "modelViewProjectionMatrix" );
        _gridUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _gridProgram, "lightPosition" );

        _lineTexture = [opengl loadTexture:@"line.jpg"];
    }

    // star points
    {
        _pointProgram = [opengl loadShader:@"PointShader"];

        [opengl generateVBAndIBFromTexturedMesh:_pointMesh vertexBuffer:_pointVertexBuffer indexBuffer:_pointIndexBuffer];

        _pointUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _pointProgram, "normalMatrix" );
        _pointUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _pointProgram, "modelViewProjectionMatrix" );
        _pointUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _pointProgram, "lightPosition" );

        _pointTexture = [opengl loadTexture:@"point.jpg"];
    }

    // floor shader, textures, VBs/IBs etc.
    {
        _floorProgram = [opengl loadShader:@"FloorShader"];

        [opengl generateVBAndIBFromTexturedMesh:_floorMesh vertexBuffer:_floorVertexBuffer indexBuffer:_floorIndexBuffer];
        
        _floorTexture = [opengl loadTexture:@"floor.jpg"];

        _floorUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _floorProgram, "normalMatrix" );
        _floorUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _floorProgram, "modelViewProjectionMatrix" );
        _floorUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _floorProgram, "lightPosition" );
    }
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];

    [opengl destroyBuffer:_stoneIndexBuffer];
    [opengl destroyBuffer:_stoneVertexBuffer];
    [opengl destroyProgram:_stoneProgramBlack];
    [opengl destroyProgram:_stoneProgramWhite];

    [opengl destroyBuffer:_boardIndexBuffer];
    [opengl destroyBuffer:_boardVertexBuffer];
    [opengl destroyProgram:_boardProgram];
    [opengl destroyTexture:_boardTexture];

    [opengl destroyProgram:_shadowProgram];
    
    [opengl destroyProgram:_gridProgram];
    [opengl destroyTexture:_lineTexture];
    
    [opengl destroyProgram:_pointProgram];
    [opengl destroyTexture:_pointTexture];

    [opengl destroyBuffer:_floorIndexBuffer];
    [opengl destroyBuffer:_floorVertexBuffer];
    [opengl destroyProgram:_floorProgram];
    [opengl destroyTexture:_floorTexture];
}

- (BOOL)canBecomeFirstResponder
{
    return YES;
}

- (void)viewDidAppear:(BOOL)animated
{
    [self becomeFirstResponder];
}

// ---------------------------------------------------------

- (void)updateTouch:(float)dt
{
    if ( _selected )
    {   
#ifndef MULTIPLE_STONES

        StoneInstance & stone = game.stones[0];

        stone.rigidBody.angularMomentum *= SelectDamping;
        
        const float select_dt = max( 1.0f / 60.0f, _selectTimestamp - _selectPrevTimestamp );

        stone.rigidBody.linearMomentum = stone.rigidBody.mass *
            ( _selectIntersectionPoint - _selectPrevIntersectionPoint ) / select_dt;

        stone.rigidBody.position = _selectIntersectionPoint + _selectOffset;

        stone.rigidBody.Activate();
    
#endif

        _holdStarted = false;
        _swipeStarted = false;
    }

    if ( _holdStarted )
    {
        _holdTime += dt;

        if ( _holdTime >= HoldDelay )
        {
            for ( int i = 0; i < game.stones.size(); ++i )
                game.stones[i].rigidBody.angularMomentum *= HoldDamping;
        }
    }

    if ( _swipeStarted )
    {
        _swipeTime += dt;

        if ( _swipeTime > MaxSwipeTime )
            _swipeStarted = false;
    }
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
//    NSLog( @"touches began" );
    
    UITouch * touch = [touches anyObject];

    if ( [touches count] == 1 )
    {
        if ( _firstPlacementTouch == nil )
        {
            _firstPlacementTouch = touch;
            _firstPlacementTimestamp = [touch timestamp];
        }

        if ( _firstPlacementTouch != nil && touch != _firstPlacementTouch && !_selected )
        {
            CGPoint selectPoint = [touch locationInView:self.view];
            
            bool onBoard = [self isPointOnBoard:selectPoint];

            if ( onBoard )
            {
                vec3f point = [self pointToPixels:selectPoint];
                
                vec3f rayStart, rayDirection;
                GetPickRay( game.inverseClipMatrix, point.x(), point.y(), rayStart, rayDirection );
                
                // next, intersect with the plane at board depth and place a new stone
                // such that it is offset from the intersection point with this plane
                
                float t = 0;
                if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), game.board.GetThickness() + game.stoneData.biconvex.GetHeight()/2, t ) )
                {
                    const float place_dt = [touch timestamp] - _firstPlacementTimestamp;

                    if ( place_dt < PlaceStoneHardThreshold )
                        telemetry.IncrementCounter( COUNTER_PlacedStoneHard );
                    else
                        telemetry.IncrementCounter( COUNTER_PlacedStone );

#ifdef MULTIPLE_STONES

                    StoneInstance stone;
                    stone.Initialize( game.stoneData );
                    stone.rigidBody.position = rayStart + rayDirection * t;
                    stone.rigidBody.Activate();
                    game.stones.push_back( stone );

#else

                    StoneInstance & stone = game.stones[0];

                    stone.rigidBody.position = rayStart + rayDirection * t;
                    stone.rigidBody.orientation = quat4f(0,0,0,1);
                    stone.rigidBody.linearMomentum = vec3f(0,0,0);
                    stone.rigidBody.angularMomentum = vec3f(0,0,0);
                    stone.rigidBody.Activate();

                    // IMPORTANT: go into select mode post placement so you can immediately drag the stone
        
                    vec3f intersectionPoint, intersectionNormal;
                    
                    if ( IntersectRayStone( game.stoneData.biconvex, stone.rigidBody.transform, rayStart, rayDirection, intersectionPoint, intersectionNormal ) > 0 )
                    {
                        _selected = true;
                        _selectTouch = touch;
                        _selectPoint = selectPoint;
                        _selectDepth = intersectionPoint.z();
                        _selectOffset = stone.rigidBody.position - intersectionPoint;
                        _selectIntersectionPoint = intersectionPoint;
                        _selectTimestamp = [touch timestamp];
                        _selectPrevIntersectionPoint = _selectIntersectionPoint;
                        _selectPrevTimestamp = _selectTimestamp;
                    }

#endif
                }
            }
        }
    }

#if !MULTIPLE_STONES

    if ( !_selected )
    {
        StoneInstance & stone = game.stones[0];

        CGPoint selectPoint = [touch locationInView:self.view];

        vec3f point = [self pointToPixels:selectPoint];

        vec3f rayStart, rayDirection;
        
        GetPickRay( inverseClipMatrix, point.x(), point.y(), rayStart, rayDirection );
        
        vec3f intersectionPoint, intersectionNormal;

        if ( IntersectRayStone( game.stoneData.biconvex, stone.rigidBody.transform, rayStart, rayDirection, intersectionPoint, intersectionNormal ) > 0 )
        {
//            NSLog( @"select" );
            
            telemetry.IncrementCounter( COUNTER_SelectedStone );

            _selected = true;
            _selectTouch = touch;
            _selectPoint = selectPoint;
            _selectDepth = intersectionPoint.z();
            _selectOffset = stone.rigidBody.position - intersectionPoint;
            _selectIntersectionPoint = intersectionPoint;
            _selectTimestamp = [touch timestamp];
            _selectPrevIntersectionPoint = _selectIntersectionPoint;
            _selectPrevTimestamp = _selectTimestamp;

            stone.rigidBody.ApplyImpulseAtWorldPoint( intersectionPoint, vec3f(0,0,-TouchImpulse) );
        }
    }

#endif

    if ( !_selected )
    {
        if ( !_holdStarted )
        {
            _holdTouch = touch;
            _holdStarted = true;
            _holdTime = 0;
            _holdPoint = [touch locationInView:self.view];
        }

        if ( !_swipeStarted )
        {
            _swipeTouch = touch;
            _swipeTime = 0;
            _swipeStarted = true;
            _swipeStartPoint = [touch locationInView:self.view];
        }
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
#ifndef MULTIPLE_STONES

    if ( _selected )
    {
        StoneInstance & stone = game.stones[0];

        if ( [touches containsObject:_selectTouch] )
        {
            UITouch * touch = _selectTouch;

            CGPoint selectPoint = [touch locationInView:self.view];
        
            vec3f point = [self pointToPixels:selectPoint];
            
            vec3f rayStart, rayDirection;
            GetPickRay( inverseClipMatrix, point.x(), point.y(), rayStart, rayDirection );
            
            // IMPORTANT: first update intersect depth. this way stones can lift
            // from the ground to the board and then not snap back to ground when dragged
            // off the board (looks bad)

            vec3f intersectionPoint, intersectionNormal;
            if ( IntersectRayStone( game.stoneData.biconvex, stone.rigidBody.transform, rayStart, rayDirection, intersectionPoint, intersectionNormal ) > 0 )
            {
                if ( intersectionPoint.z() > _selectDepth )
                {
                    _selectPoint = selectPoint;
                    _selectDepth = intersectionPoint.z();
                    _selectOffset = stone.rigidBody.position - intersectionPoint;
                    _selectIntersectionPoint = intersectionPoint;
                }
            }

            // next, intersect with the plane at select depth and move the stone
            // such that it is offset from the intersection point with this plane

            float t = 0;
            if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), _selectDepth, t ) )
            {
                _selectPrevIntersectionPoint = _selectIntersectionPoint;
                _selectPrevTimestamp = _selectTimestamp;

                _selectIntersectionPoint = rayStart + rayDirection * t;
                _selectTimestamp = [touch timestamp];
            }
        }
    }
    else
#endif
    {
        if ( _holdStarted && [touches containsObject:_holdTouch] )
        {
            UITouch * touch = _holdTouch;

            _holdStarted = false;

            CGPoint currentPosition = [touch locationInView:self.view];

            vec3f delta( _holdPoint.x - currentPosition.x,
                         _holdPoint.y - currentPosition.y,
                         0 );

            if ( length( delta ) > HoldMoveThreshold )
            {
//                NSLog( @"hold moved" );
                _holdStarted = false;
            }
        }

        if ( _swipeStarted && [touches containsObject:_swipeTouch] )
        {
            UITouch * touch = _swipeTouch;
            
            CGPoint currentPosition = [touch locationInView:self.view];

            vec3f swipeDelta( _swipeStartPoint.x - currentPosition.x,
                              _swipeStartPoint.y - currentPosition.y,
                              0 );
            
            if ( length( swipeDelta ) >= MinimumSwipeLength + SwipeLengthPerSecond * _swipeTime )
            {
                game.OnSwipe( [self pointToPixels:_swipeStartPoint] );
                
                _swipeStarted = false;
            }
        }
    }
}

- (vec3f) pointToPixels:(CGPoint)point
{
    const float contentScaleFactor = [self.view contentScaleFactor];
    vec3f pixels( point.x, point.y, 0 );
    pixels *= contentScaleFactor;
    return pixels;
}

- (bool) isPointOnBoard:(CGPoint)selectPoint
{
    return game.IsScreenPointOnBoard( [self pointToPixels:selectPoint] );
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
//    NSLog( @"touches ended" );
    
    if ( _firstPlacementTouch != nil && [touches containsObject:_firstPlacementTouch] )
        _firstPlacementTouch = nil;
    
    UITouch * touch = [touches anyObject];

    if ( touch.tapCount >= 2 )
    {
        CGPoint touchPoint = [touch locationInView:self.view];

        game.OnDoubleTap( [self pointToPixels:touchPoint] );
        
        [NSObject cancelPreviousPerformRequestsWithTarget:self];
    }

#ifndef MULTIPLE_STONES

    if ( _selected )
    {
        StoneInstance & stone = game.stones[0];

        if ( [touches containsObject:_selectTouch] )
        {
            UITouch * touch = _selectTouch;

            CGPoint selectPoint = [touch locationInView:self.view];

            const bool onBoard = [self isPointOnBoard:selectPoint];

            const float dx = selectPoint.x - _selectPoint.x;
            const float dy = selectPoint.y - _selectPoint.y;

            const float distance = sqrt( dx*dx + dy*dy );

            const float speed = length( stone.rigidBody.linearVelocity );

            if ( game.locked && onBoard && speed > LockedDragSpeedMax )
            {
                stone.rigidBody.linearVelocity /= speed;
                stone.rigidBody.linearVelocity *= LockedDragSpeedMax;
                stone.rigidBody.linearMomentum = stone.rigidBody.linearVelocity * stone.rigidBody.mass;
                stone.rigidBody.UpdateMomentum();
            }

            stone.rigidBody.Activate();
            
            if ( distance > 10 )
            {
                if ( speed > FlickSpeedThreshold )
                    telemetry.IncrementCounter( COUNTER_FlickedStone );
                else
                    telemetry.IncrementCounter( COUNTER_DraggedStone );
            }
            else
            {
                const float currentTimestamp = [touch timestamp];
                if ( currentTimestamp - _selectTimestamp < 0.1f )
                    telemetry.IncrementCounter( COUNTER_TouchedStone );
            }
        }
    }
#endif

    if ( [touches containsObject:_selectTouch] )
        _selected = false;

    if ( [touches containsObject:_holdTouch] )
        _holdStarted = false;

    if ( [touches containsObject:_swipeTouch] )
        _swipeStarted = false;
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    _holdStarted = false;
    _swipeStarted = false;
    _selected = false;
    _firstPlacementTouch = nil;
}

- (void)accelerometer:(UIAccelerometer *)uiAccelerometer didAccelerate:(UIAcceleration *)acceleration
{
    accelerometer.Update( vec3f( acceleration.x, acceleration.y, acceleration.z ) );
}

- (void)render
{    
    glClearColor( 0, 0, 0, 1 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    if ( game.paused )
        return;

    // render floor

    {
        glUseProgram( _floorProgram );
                
        [opengl selectTexturedMesh:_floorTexture vertexBuffer:_floorVertexBuffer indexBuffer:_floorIndexBuffer];

        glUniformMatrix4fv( _floorUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&game.clipMatrix );
        glUniformMatrix3fv( _floorUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&game.normalMatrix );
        glUniform3fv( _floorUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.lightPosition );

        glDrawElements( GL_TRIANGLES, _floorMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
    }

    // render board

    {
        glUseProgram( _boardProgram );
        
        [opengl selectTexturedMesh:_boardTexture vertexBuffer:_boardVertexBuffer indexBuffer:_boardIndexBuffer];

        glUniformMatrix4fv( _boardUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&game.clipMatrix );
        glUniformMatrix3fv( _boardUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&game.normalMatrix );
        glUniform3fv( _boardUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.lightPosition );

        glDrawElements( GL_TRIANGLES, _boardMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
    }

    // render grid
    
    {
        glUseProgram( _gridProgram );

        [opengl selectTexturedMesh:_lineTexture vertexBuffer:_gridVertexBuffer indexBuffer:_gridIndexBuffer];
        
        glUniformMatrix4fv( _gridUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&game.clipMatrix );
        glUniformMatrix3fv( _gridUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&game.normalMatrix );
        glUniform3fv( _gridUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.lightPosition );
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_ZERO, GL_SRC_COLOR );

        glDisable( GL_DEPTH_TEST );

        glDrawElements( GL_TRIANGLES, _gridMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        
        glEnable( GL_DEPTH_TEST );

        glDisable( GL_BLEND );
    }

    // render star points
    
    {
        glUseProgram( _pointProgram );
        
        [opengl selectTexturedMesh:_pointTexture vertexBuffer:_pointVertexBuffer indexBuffer:_pointIndexBuffer];

        glUniformMatrix4fv( _pointUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&game.clipMatrix );
        glUniformMatrix3fv( _pointUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&game.normalMatrix );
        glUniform3fv( _pointUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.lightPosition );
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_ZERO, GL_SRC_COLOR );

        glDisable( GL_DEPTH_TEST );

        glDrawElements( GL_TRIANGLES, _pointMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        
        glEnable( GL_DEPTH_TEST );

        glDisable( GL_BLEND );
    }

    // render board shadow on ground
    
    {
        glUseProgram( _shadowProgram );
        
        [opengl selectTexturedMesh:0 vertexBuffer:_boardVertexBuffer indexBuffer:_boardIndexBuffer];

        float boardShadowAlpha = 1.0f;

        mat4f shadowMatrix;
        MakeShadowMatrix( vec4f(0,0,1,-0.1f), vec4f( game.lightPosition.x(), game.lightPosition.y(), game.lightPosition.z() * 0.5f, 0 ), shadowMatrix );

        mat4f modelView = game.cameraMatrix * shadowMatrix;
        mat4f modelViewProjectionMatrix = game.projectionMatrix * modelView;
        
        glUniformMatrix4fv( _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&modelViewProjectionMatrix );
        glUniform1fv( _shadowUniforms[UNIFORM_ALPHA], 1, (float*)&boardShadowAlpha );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        
        glDrawElements( GL_TRIANGLES, _boardMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );

        glDisable( GL_BLEND );
    }

    // render stone shadow on ground
    
    {
        glUseProgram( _shadowProgram );
        
        [opengl selectNonTexturedMesh:_stoneVertexBuffer indexBuffer:_stoneIndexBuffer];

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        for ( int i = 0; i < game.stones.size(); ++i )
        {
            StoneInstance & stone = game.stones[i];
            
            mat4f shadowMatrix;
            MakeShadowMatrix( vec4f(0,0,1,0), vec4f( game.lightPosition.x(), game.lightPosition.y(), game.lightPosition.z(), 1 ), shadowMatrix );

            mat4f modelView = game.cameraMatrix * shadowMatrix * stone.rigidBody.transform.localToWorld;
            mat4f modelViewProjectionMatrix = game.projectionMatrix * modelView;
            
            glUniformMatrix4fv( _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&modelViewProjectionMatrix );

            float shadowAlpha = GetShadowAlpha( stone.rigidBody.position );
            glUniform1fv( _shadowUniforms[UNIFORM_ALPHA], 1, (float*)&shadowAlpha );

            glDrawElements( GL_TRIANGLES, _stoneMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        }

        glDisable( GL_BLEND );
        
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        
        glDisableVertexAttribArray( GLKVertexAttribPosition );
        glDisableVertexAttribArray( GLKVertexAttribNormal );
    }

    // render stone shadow on board
    
    {
        glUseProgram( _shadowProgram );
        
        [opengl selectNonTexturedMesh:_stoneVertexBuffer indexBuffer:_stoneIndexBuffer];
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_GREATER );

        for ( int i = 0; i < game.stones.size(); ++i )
        {
            StoneInstance & stone = game.stones[i];

            if ( stone.rigidBody.position.z() < game.board.GetThickness() )
                continue;

            mat4f shadowMatrix;
            MakeShadowMatrix( vec4f(0,0,1,-game.board.GetThickness()+0.1f), vec4f( game.lightPosition.x(), game.lightPosition.y(), game.lightPosition.z(), 1 ), shadowMatrix );
            
            mat4f modelView = game.cameraMatrix * shadowMatrix * stone.rigidBody.transform.localToWorld;
            mat4f modelViewProjectionMatrix = game.projectionMatrix * modelView;
            
            glUniformMatrix4fv( _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&modelViewProjectionMatrix );
            
            float shadowAlpha = GetShadowAlpha( stone.rigidBody.position );
            glUniform1fv( _shadowUniforms[UNIFORM_ALPHA], 1, (float*)&shadowAlpha );

            glDrawElements( GL_TRIANGLES, _stoneMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        }

        glDepthFunc( GL_LESS );

        glDisable( GL_BLEND );
    }

    // render white stones

    {
        glUseProgram( _stoneProgramWhite );
                
        [opengl selectNonTexturedMesh:_stoneVertexBuffer indexBuffer:_stoneIndexBuffer];

        for ( int i = 0; i < game.stones.size(); ++i )
        {
            StoneInstance & stone = game.stones[i];
            
            if ( !stone.white )
                continue;
            
            mat4f modelViewMatrix = game.cameraMatrix * stone.rigidBody.transform.localToWorld;
            
            mat3f stoneNormalMatrix;
            stoneNormalMatrix.load( modelViewMatrix );

            mat4f stoneModelViewProjectionMatrix = game.projectionMatrix * modelViewMatrix;

            glUniformMatrix4fv( _stoneUniformsWhite[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&stoneModelViewProjectionMatrix );
            glUniformMatrix3fv( _stoneUniformsWhite[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&stoneNormalMatrix );
            glUniform3fv( _stoneUniformsWhite[UNIFORM_LIGHT_POSITION], 1, (float*)&game.lightPosition );

            glDrawElements( GL_TRIANGLES, _stoneMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        }
    }

    // render black stones
    
    {
        glUseProgram( _stoneProgramBlack );
        
        [opengl selectNonTexturedMesh:_stoneVertexBuffer indexBuffer:_stoneIndexBuffer];
        
        for ( int i = 0; i < game.stones.size(); ++i )
        {
            StoneInstance & stone = game.stones[i];
            
            if ( stone.white )
                continue;
            
            mat4f modelViewMatrix = game.cameraMatrix * stone.rigidBody.transform.localToWorld;
            
            mat3f stoneNormalMatrix;
            stoneNormalMatrix.load( modelViewMatrix );
            
            mat4f stoneModelViewProjectionMatrix = game.projectionMatrix * modelViewMatrix;
            
            glUniformMatrix4fv( _stoneUniformsBlack[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&stoneModelViewProjectionMatrix );
            glUniformMatrix3fv( _stoneUniformsBlack[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&stoneNormalMatrix );
            glUniform3fv( _stoneUniformsBlack[UNIFORM_LIGHT_POSITION], 1, (float*)&game.lightPosition );
            
            glDrawElements( GL_TRIANGLES, _stoneMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        }
    }

    const GLenum discards[]  = { GL_DEPTH_ATTACHMENT };
    glDiscardFramebufferEXT( GL_FRAMEBUFFER, 1, discards );
}

- (void)update
{
    float dt = 1.0f / 60.0f;

    if ( game.paused )
        dt = 0.0f;

    game.UpdateMatrices();

    [self updateTouch:dt];

    game.UpdatePhysics( dt, accelerometer );
    
    [self render];

    telemetry.Update( dt, game.board, game.stones, game.locked, accelerometer.GetUp() );
}

@end
