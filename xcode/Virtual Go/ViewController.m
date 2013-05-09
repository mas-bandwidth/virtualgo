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

    GLKMatrix4 _clipMatrix;
    GLKMatrix4 _inverseClipMatrix;

    Mesh<Vertex> _stoneMesh;
    GLuint _stoneProgram;
    GLuint _stoneVertexBuffer;
    GLuint _stoneIndexBuffer;
    GLint _stoneUniforms[NUM_UNIFORMS];
    
    Mesh<TexturedVertex> _boardMesh;
    GLuint _boardProgram;
    GLuint _boardTexture;
    GLuint _boardVertexBuffer;
    GLuint _boardIndexBuffer;
    GLKMatrix4 _boardModelViewProjectionMatrix;
    GLKMatrix3 _boardNormalMatrix;
    GLint _boardUniforms[NUM_UNIFORMS];

    GLuint _shadowProgram;
    GLint _shadowUniforms[NUM_UNIFORMS];

    Mesh<TexturedVertex> _gridMesh;
    GLuint _gridProgram;
    GLuint _gridVertexBuffer;
    GLuint _gridIndexBuffer;
    GLKMatrix4 _gridModelViewProjectionMatrix;
    GLKMatrix3 _gridNormalMatrix;
    GLint _gridUniforms[NUM_UNIFORMS];
    GLuint _lineTexture;

    Mesh<TexturedVertex> _pointMesh;
    GLuint _pointProgram;
    GLuint _pointVertexBuffer;
    GLuint _pointIndexBuffer;
    GLKMatrix4 _pointModelViewProjectionMatrix;
    GLKMatrix3 _pointNormalMatrix;
    GLint _pointUniforms[NUM_UNIFORMS];
    GLuint _pointTexture;

    Mesh<TexturedVertex> _floorMesh;
    GLuint _floorProgram;
    GLuint _floorTexture;
    GLuint _floorVertexBuffer;
    GLuint _floorIndexBuffer;
    GLKMatrix4 _floorModelViewProjectionMatrix;
    GLKMatrix3 _floorNormalMatrix;
    GLint _floorUniforms[NUM_UNIFORMS];

    bool _hasRendered;      // todo: remove this by splitting the code that calculates matrices from render

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
    telemetry.SetCounterNotifyFunc( HandleCounterNotify );

    [super viewDidLoad];

    UIAccelerometer * uiAccelerometer = [UIAccelerometer sharedAccelerometer];
    uiAccelerometer.updateInterval = 1 / AccelerometerFrequency;
    uiAccelerometer.delegate = self;
    
    self.context = [ [EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 ];

    if (!self.context)
    {
        NSLog( @"Failed to create ES context" );
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    
    game.board.Initialize( BoardSize );

    game.stoneData.Initialize( STONE_SIZE_40 );
    
    GenerateBiconvexMesh( _stoneMesh, game.stoneData.biconvex, 3 );

    GenerateFloorMesh( _floorMesh );
    GenerateBoardMesh( _boardMesh, game.board );
    GenerateGridMesh( _gridMesh, game.board );
    GenerateStarPointsMesh( _pointMesh, game.board );

    opengl = [[OpenGL alloc] init];
   
    [self setupGL];
  
    game.locked = false;//true;
    game.paused = false;
    game.zoomed = !iPad();
    
    game.smoothZoom = [self getTargetZoom];
    
    _hasRendered = false;

    _swipeStarted = false;
    _holdStarted = false;
    _selected = false;

    _firstPlacementTouch = nil;
    
    [self updateLights];

#if MULTIPLE_STONES

    for ( int i = 1; i <= BoardSize; ++i )
    {
        for ( int j = 1; j <= BoardSize; ++j )
        {
            StoneInstance stone;
            stone.Initialize( game.stoneData );
            stone.rigidBody.position = _board.GetPointPosition( i, j ) + vec3f( 0, 0, stone.biconvex.GetHeight() / 2 );
            stone.rigidBody.orientation = quat4f(1,0,0,0);
            stone.rigidBody.linearMomentum = vec3f(0,0,0);
            stone.rigidBody.angularMomentum = vec3f(0,0,0);
            stone.rigidBody.Activate();
            game.stones.push_back( stone );
        }
    }

#else

    StoneInstance stone;
    stone.Initialize( game.stoneData );
    stone.rigidBody.position = vec3f( 0, 0, game.board.GetThickness() + game.stoneData.biconvex.GetHeight()/2 );
    stone.rigidBody.orientation = quat4f(1,0,0,0);
    stone.rigidBody.linearMomentum = vec3f(0,0,0);
    stone.rigidBody.angularMomentum = vec3f(0,0,0);
    stone.rigidBody.Activate();
    game.stones.push_back( stone );

#endif
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
        
    glEnable( GL_DEPTH_TEST );

    glDepthFunc( GL_LESS );

    glEnable( GL_CULL_FACE );

    // stone shader, textures, VBs/IBs etc.
    {    
        _stoneProgram = [opengl loadShader:@"StoneShader"];

        [opengl generateVBAndIBFromMesh:_stoneMesh vertexBuffer:_stoneVertexBuffer indexBuffer:_stoneIndexBuffer];

        _stoneUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _stoneProgram, "normalMatrix" );
        _stoneUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _stoneProgram, "modelViewProjectionMatrix" );
        _stoneUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _stoneProgram, "lightPosition" );
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

    // configure opengl view

    self.preferredFramesPerSecond = 60;

    self.view.multipleTouchEnabled = YES;
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];

    [opengl destroyBuffer:_stoneIndexBuffer];
    [opengl destroyBuffer:_stoneVertexBuffer];
    [opengl destroyProgram:_stoneProgram];

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

// -------------------

- (void)toggleLocked
{
    telemetry.IncrementCounter( COUNTER_ToggleLocked );
    game.locked = !game.locked;
}

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
                // IMPORTANT: convert points to pixels!
                const float contentScaleFactor = [self.view contentScaleFactor];
                vec3f point( selectPoint.x, selectPoint.y, 0 );
                point *= contentScaleFactor;
                
                vec3f rayStart, rayDirection;
                
                mat4f inverseClipMatrix;
                inverseClipMatrix.load( _inverseClipMatrix.m );
                
                GetPickRay( inverseClipMatrix, point.x(), point.y(), rayStart, rayDirection );
                
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

                    Stone stone;
                    stone.Initialize( STONE_SIZE_40 );
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

        // IMPORTANT: convert points to pixels!
        const float contentScaleFactor = [self.view contentScaleFactor];
        vec3f point( selectPoint.x, selectPoint.y, 0 );
        point *= contentScaleFactor; 

        vec3f rayStart, rayDirection;
        
        mat4f inverseClipMatrix;
        inverseClipMatrix.load( _inverseClipMatrix.m );
        
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
        
            // IMPORTANT: convert points to pixels!
            const float contentScaleFactor = [self.view contentScaleFactor];
            vec3f point( selectPoint.x, selectPoint.y, 0 );
            point *= contentScaleFactor;
            
            vec3f rayStart, rayDirection;
            
            mat4f inverseClipMatrix;
            inverseClipMatrix.load( _inverseClipMatrix.m );
            
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

            vec3f swipePoint( _swipeStartPoint.x, _swipeStartPoint.y, 0 );
            
            vec3f swipeDelta( _swipeStartPoint.x - currentPosition.x,
                              _swipeStartPoint.y - currentPosition.y,
                              0 );
            
            if ( length( swipeDelta ) >= MinimumSwipeLength + SwipeLengthPerSecond * _swipeTime )
            {
                // IMPORTANT: convert points to pixels!
                const float contentScaleFactor = [self.view contentScaleFactor];
                swipePoint *= contentScaleFactor;
                swipeDelta *= contentScaleFactor;
                
                [self handleSwipe:swipeDelta atPoint:swipePoint ];

                _swipeStarted = false;
            }
        }
    }
}

// todo: can move this out to GameInstance.h -- keep it general, and in pixels!

- (bool) isPointOnBoard:(CGPoint)selectPoint
{
    const float contentScaleFactor = [self.view contentScaleFactor];
    vec3f point( selectPoint.x, selectPoint.y, 0 );
    point *= contentScaleFactor;
    
    vec3f rayStart, rayDirection;
    
    mat4f inverseClipMatrix;
    inverseClipMatrix.load( _inverseClipMatrix.m );
    
    GetPickRay( inverseClipMatrix, point.x(), point.y(), rayStart, rayDirection );
    
    // next, intersect with the plane at select depth and move the stone
    // such that it is offset from the intersection point with this plane
    
    float t = 0;
    if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), game.board.GetThickness(), t ) )
    {
        vec3f intersectionPoint = rayStart + rayDirection * t;

        const float x = intersectionPoint.x();
        const float y = intersectionPoint.y();

        float bx, by;
        game.board.GetBounds( bx, by );

        return x >= -bx && x <= bx && y >= -by && y <= by;
    }

    return false;
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
//    NSLog( @"touches ended" );
    
    if ( _firstPlacementTouch != nil && [touches containsObject:_firstPlacementTouch] )
        _firstPlacementTouch = nil;
    
    UITouch * touch = [touches anyObject];
    
    if ( touch.tapCount == 1 )
    {
        NSDictionary * touchLoc = [NSDictionary dictionaryWithObject:
                                   [NSValue valueWithCGPoint:[touch locationInView:self.view]] forKey:@"location"];
        
        [self performSelector:@selector(handleSingleTap:) withObject:touchLoc afterDelay:0.3];
        
    }
    else if ( touch.tapCount >= 2 )
    {
//            NSLog( @"double tap" );

        if ( iPad() && _firstPlacementTouch == nil )
        {
            if ( game.zoomed )
                telemetry.IncrementCounter( COUNTER_ZoomedOut );
            else
                telemetry.IncrementCounter( COUNTER_ZoomedIn );
        
            game.zoomed = !game.zoomed;
        }

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

- (void)handleSingleTap:(NSDictionary *)touches
{
//    NSLog( @"single tap" );
}

- (void)handleSwipe:(vec3f)delta atPoint:(vec3f)point
{
    #if !LOCKED
    if ( !game.locked )
    {
//      NSLog( @"swipe" );

        for ( int i = 0; i < game.stones.size(); ++i )
        {
            StoneInstance & stone = game.stones[i];

            const vec3f & up = accelerometer.GetUp();
            stone.rigidBody.angularMomentum += SwipeMomentum * up;
            stone.rigidBody.Activate();
        }

        telemetry.IncrementCounter( COUNTER_Swiped );

        telemetry.SetSwipedThisFrame();
    }
    #endif
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

- (void)updatePhysics:(float)dt
{
    if ( !_hasRendered )
        return;

    // calculate frustum planes for collision

    mat4f clipMatrix;
    clipMatrix.load( _clipMatrix.m );

    Frustum frustum;
    CalculateFrustumPlanes( clipMatrix, frustum );
    
    // apply jerk acceleration to stones

    const vec3f & jerkAcceleration = accelerometer.GetJerkAcceleration();
    const float jerk = length( jerkAcceleration );
    if ( jerk > JerkThreshold )
    {
        for ( int i = 0; i < game.stones.size(); ++i )
        {
            StoneInstance & stone = game.stones[i];
            stone.rigidBody.ApplyImpulse( JerkScale * jerkAcceleration * stone.rigidBody.mass );
        }
    }

    // detect when the user has asked to launch the stone into the air

    const vec3f down = accelerometer.GetDown();
    const vec3f up = -down;

    if ( !game.locked )
    {
        if ( iPad() )
        {
            // for pads, let the launch go up in the air only!
            // other launches feel wrong and cause nausea -- treat the tablet like the board
            
            const float jerkUp = dot( jerkAcceleration, up );
            
            if ( jerkUp > LaunchThreshold )
            {
                for ( int i = 0; i < game.stones.size(); ++i )
                {
                    StoneInstance & stone = game.stones[i];
                    stone.rigidBody.ApplyImpulse( jerkUp * LaunchMomentum * up );
                }

                telemetry.IncrementCounter( COUNTER_AppliedImpulse );
            }
        }
        else
        {
            // for the iphone let the player shake the stone around like a toy
            // the go stone is trapped inside the phone. this looks cool!
            
            if ( jerk > LaunchThreshold )
            {
                for ( int i = 0; i < game.stones.size(); ++i )
                {
                    StoneInstance & stone = game.stones[i];
                    stone.rigidBody.ApplyImpulse( jerkAcceleration * LaunchMomentum );
                }

                telemetry.IncrementCounter( COUNTER_AppliedImpulse );
            }
        }
    }
 
    // update physics sim

    vec3f gravity = 10 * 9.8f * accelerometer.GetDown();
    
    UpdatePhysics( dt, game.board, game.stoneData, game.stones, telemetry,
                   frustum, gravity, _selected, game.locked, game.smoothZoom );
 }

- (void)updateLights
{
    game.lightPosition = vec3f( 30, 30, 100 );
}

- (float)getTargetZoom
{
    return iPad() ? ( game.zoomed ? ZoomIn_iPad : ZoomOut_iPad ) : ( game.zoomed ? ZoomIn_iPhone : ZoomOut_iPhone );
}

- (void)render
{
    float aspect = fabsf( self.view.bounds.size.width / self.view.bounds.size.height );
    
    const float targetZoom = [self getTargetZoom];
    
    game.smoothZoom += ( targetZoom - game.smoothZoom ) * ( game.zoomed ? ZoomInTightness : ZoomOutTightness );
    
    GLKMatrix4 projectionMatrix = GLKMatrix4MakePerspective( GLKMathDegreesToRadians(40.0f), aspect, 0.1f, 100.0f );
    
    GLKMatrix4 baseModelViewMatrix = GLKMatrix4MakeLookAt( 0, 0, game.smoothZoom,
                                                           0, 0, 0,
                                                           0, 1, 0 );

    _clipMatrix = GLKMatrix4Multiply( projectionMatrix, baseModelViewMatrix );

    bool invertible;
    _inverseClipMatrix = GLKMatrix4Invert( _clipMatrix, &invertible );
    assert( invertible );

    // todo: need to clean up this matrix bullshit
    
    _boardNormalMatrix = GLKMatrix3InvertAndTranspose( GLKMatrix4GetMatrix3(baseModelViewMatrix), NULL );
    _boardModelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, baseModelViewMatrix );
    
    _gridNormalMatrix = _boardNormalMatrix;
    _gridModelViewProjectionMatrix = _boardModelViewProjectionMatrix;

    _pointNormalMatrix = _boardNormalMatrix;
    _pointModelViewProjectionMatrix = _boardModelViewProjectionMatrix;

    _floorNormalMatrix = GLKMatrix3InvertAndTranspose( GLKMatrix4GetMatrix3(baseModelViewMatrix), NULL );
    _floorModelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, baseModelViewMatrix );

    glClearColor( 0, 0, 0, 1 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    if ( game.paused )
        return;

    // render floor

    {
        glUseProgram( _floorProgram );
                
        [opengl selectTexturedMesh:_floorTexture vertexBuffer:_floorVertexBuffer indexBuffer:_floorIndexBuffer];

        glUniformMatrix4fv( _floorUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _floorModelViewProjectionMatrix.m );
        glUniformMatrix3fv( _floorUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _floorNormalMatrix.m );
        glUniform3fv( _floorUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.lightPosition );

        glDrawElements( GL_TRIANGLES, _floorMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
    }

    // render board

    {
        glUseProgram( _boardProgram );
        
        [opengl selectTexturedMesh:_boardTexture vertexBuffer:_boardVertexBuffer indexBuffer:_boardIndexBuffer];

        glUniformMatrix4fv( _boardUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _boardModelViewProjectionMatrix.m );
        glUniformMatrix3fv( _boardUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _boardNormalMatrix.m );
        glUniform3fv( _boardUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.lightPosition );

        glDrawElements( GL_TRIANGLES, _boardMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
    }

    // render grid
    
    {
        glUseProgram( _gridProgram );

        [opengl selectTexturedMesh:_lineTexture vertexBuffer:_gridVertexBuffer indexBuffer:_gridIndexBuffer];
        
        glUniformMatrix4fv( _gridUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _gridModelViewProjectionMatrix.m );
        glUniformMatrix3fv( _gridUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _gridNormalMatrix.m );
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

        glUniformMatrix4fv( _pointUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _pointModelViewProjectionMatrix.m );
        glUniformMatrix3fv( _pointUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _pointNormalMatrix.m );
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
        
        [opengl selectNonTexturedMesh:_boardVertexBuffer indexBuffer:_boardIndexBuffer];

        float boardShadowAlpha = 1.0f;

        GLKMatrix4 view = baseModelViewMatrix;

        // todo: clean up this bullshit. should be directly working with mat4f the whole time
        mat4f shadow_matrix;
        MakeShadowMatrix( vec4f(0,0,1,-0.1f), vec4f( game.lightPosition.x(), game.lightPosition.y(), game.lightPosition.z() * 0.5f, 0 ), shadow_matrix );
        float shadow_data[16];
        shadow_matrix.store( shadow_data );
        GLKMatrix4 shadow = GLKMatrix4MakeWithArray( shadow_data );

        GLKMatrix4 modelView = GLKMatrix4Multiply( view, shadow );

        GLKMatrix4 boardGroundShadowModelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, modelView );
        
        glUniformMatrix4fv( _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, boardGroundShadowModelViewProjectionMatrix.m );
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
            
            GLKMatrix4 view = baseModelViewMatrix;
            
            float opengl_transform[16];
            stone.rigidBody.transform.localToWorld.store( opengl_transform );

            GLKMatrix4 model = GLKMatrix4MakeWithArray( opengl_transform );
            
            // todo: clean up this bullshit
            mat4f shadow_matrix;
            MakeShadowMatrix( vec4f(0,0,1,0), vec4f( game.lightPosition.x(), game.lightPosition.y(), game.lightPosition.z(), 1 ), shadow_matrix );
            float shadow_data[16];
            shadow_matrix.store( shadow_data );
            GLKMatrix4 shadow = GLKMatrix4MakeWithArray( shadow_data );

            GLKMatrix4 modelView = GLKMatrix4Multiply( view, GLKMatrix4Multiply( shadow, model ) );

            GLKMatrix4 stoneGroundShadowModelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, modelView );

            glUniformMatrix4fv( _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, stoneGroundShadowModelViewProjectionMatrix.m );

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
        
            float shadowAlpha = GetShadowAlpha( stone.rigidBody.position );
            
            if ( shadowAlpha == 0.0f )
                continue;

            GLKMatrix4 view = baseModelViewMatrix;
            
            float opengl_transform[16];
            stone.rigidBody.transform.localToWorld.store( opengl_transform );

            GLKMatrix4 model = GLKMatrix4MakeWithArray( opengl_transform );
            
            // HACK: the fact that I have to minus here indicates
            // something wrong in the shadow matrix derivation. perhaps it assumes left handed?
            mat4f shadow_matrix;
            MakeShadowMatrix( vec4f(0,0,1,-game.board.GetThickness()+0.1f), vec4f( game.lightPosition.x(), game.lightPosition.y(), game.lightPosition.z(), 1 ), shadow_matrix );
            float shadow_data[16];
            shadow_matrix.store( shadow_data );
            GLKMatrix4 shadow = GLKMatrix4MakeWithArray( shadow_data );

            GLKMatrix4 modelView = GLKMatrix4Multiply( view, GLKMatrix4Multiply( shadow, model ) );

            GLKMatrix4 stoneBoardShadowModelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, modelView );

            glUniformMatrix4fv( _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, stoneBoardShadowModelViewProjectionMatrix.m );

            glUniform1fv( _shadowUniforms[UNIFORM_ALPHA], 1, (float*)&shadowAlpha );

            glDrawElements( GL_TRIANGLES, _stoneMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        }

        glDepthFunc( GL_LESS );

        glDisable( GL_BLEND );
    }

    // render stone

    {
        glUseProgram( _stoneProgram );
                
        [opengl selectNonTexturedMesh:_stoneVertexBuffer indexBuffer:_stoneIndexBuffer];

        for ( int i = 0; i < game.stones.size(); ++i )
        {
            StoneInstance & stone = game.stones[i];
            
            // todo: standardize all matrix math to use vectorial (portable)
            // instead of using GLKMatrix4 bullshit (not portable)
            float opengl_transform[16];
            stone.rigidBody.transform.localToWorld.store( opengl_transform );

            GLKMatrix4 modelViewMatrix = GLKMatrix4MakeWithArray( opengl_transform );
            modelViewMatrix = GLKMatrix4Multiply( baseModelViewMatrix, modelViewMatrix );
            
            GLKMatrix3 stoneNormalMatrix = GLKMatrix3InvertAndTranspose( GLKMatrix4GetMatrix3(modelViewMatrix), NULL );
            GLKMatrix4 stoneModelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, modelViewMatrix );

            glUniformMatrix4fv( _stoneUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, stoneModelViewProjectionMatrix.m );
            glUniformMatrix3fv( _stoneUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, stoneNormalMatrix.m );
            glUniform3fv( _stoneUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.lightPosition );

            glDrawElements( GL_TRIANGLES, _stoneMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        }
    }

    const GLenum discards[]  = { GL_DEPTH_ATTACHMENT };
    glDiscardFramebufferEXT( GL_FRAMEBUFFER, 1, discards );
    
    _hasRendered = true;
}

- (void)update
{
    float dt = 1.0f / 60.0f;

    if ( game.paused )
        dt = 0.0f;

    [self updateTouch:dt];

    [self updatePhysics:dt];
    
    [self updateLights];

    [self render];

    telemetry.Update( dt, game.board, game.stones, game.locked, accelerometer.GetUp() );
}

@end
