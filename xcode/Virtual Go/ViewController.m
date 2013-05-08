//
//  ViewController.m
//  Virtual Go
//
//  Created by Glenn Fiedler on 4/13/13.
//  Copyright (c) 2013 Glenn Fiedler. All rights reserved.
//

#import "ViewController.h"
#import "testflight/TestFlight.h"

#include "../../Config.h"
#include "Common.h"
#include "Biconvex.h"
#include "Stone.h"
#include "Board.h"
#include "CollisionDetection.h"
#include "CollisionResponse.h"
#include "../../Telemetry.h"
#include "../../Render.h"
#include "../../Accelerometer.h"
#include "../../MeshGenerators.h"

@interface ViewController ()
{    
    GLKMatrix4 _clipMatrix;
    GLKMatrix4 _inverseClipMatrix;
        
    std::vector<Stone> _stones;
    Biconvex _biconvex;
    Mesh<Vertex> _stoneMesh;
    GLuint _stoneProgram;
    GLuint _stoneVertexBuffer;
    GLuint _stoneIndexBuffer;
    GLint _stoneUniforms[NUM_UNIFORMS];
    
    Board _board;
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

    vec3f _lightPosition;

    bool _locked;
    bool _paused;
    bool _zoomed;
    bool _hasRendered;

    float _smoothZoom;

    Accelerometer _accelerometer;    

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
        
    uint64_t _counters[COUNTER_NumValues];
    
    uint32_t _collisionPlanes;

    double _secondsSinceCollision[COLLISION_NumValues];

    bool _swipedThisFrame;
    double _secondsSinceLastSwipe;
    
    DetectionTimer _detectionTimer[CONDITION_NumValues];
}

@property (strong, nonatomic) EAGLContext *context;

- (void)setupGL;
- (void)tearDownGL;

- (void)didBecomeActive:(NSNotification *)notification;
- (void)willResignActive:(NSNotification *)notification;
- (void)didEnterBackground:(NSNotification *)notification;
- (void)willEnterForeground:(NSNotification *)notification;

- (void)deviceOrientationDidChange:(NSNotification *)notification;

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event;

- (GLuint)loadShader:(NSString*)filename;
- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file;
- (BOOL)linkProgram:(GLuint)prog;
- (BOOL)validateProgram:(GLuint)prog;

@end

@implementation ViewController

bool iPad()
{
    return UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    [self setupNotifications];

    UIAccelerometer * accelerometer = [UIAccelerometer sharedAccelerometer];
    accelerometer.updateInterval = 1 / AccelerometerFrequency;
    accelerometer.delegate = self;
    
    self.context = [ [EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 ];

    if (!self.context)
    {
        NSLog( @"Failed to create ES context" );
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    
    _board.Initialize( BoardSize );

    Stone stone;
    stone.Initialize( STONE_SIZE_40 );
    GenerateBiconvexMesh( _stoneMesh, stone.biconvex, 3 );
    _biconvex = stone.biconvex;

    GenerateFloorMesh( _floorMesh );
    GenerateBoardMesh( _boardMesh, _board );
    GenerateGridMesh( _gridMesh, _board );
    GenerateStarPointsMesh( _pointMesh, _board );

    [self setupGL];
  
    _locked = false;//true;
    _paused = true;
    _zoomed = !iPad();
    _hasRendered = false;
    
    _smoothZoom = [self getTargetZoom];
    
    _swipeStarted = false;
    _holdStarted = false;
    _selected = false;

    _firstPlacementTouch = nil;
    
    _collisionPlanes = 0;

    memset( _secondsSinceCollision, 0, sizeof( _secondsSinceCollision ) );

    _swipedThisFrame = false;
    _secondsSinceLastSwipe = 0;

    [self updateLights];

#if MULTIPLE_STONES

    for ( int i = 1; i <= BoardSize; ++i )
    {
        for ( int j = 1; j <= BoardSize; ++j )
        {
            stone.rigidBody.position = _board.GetPointPosition( i, j ) + vec3f( 0, 0, stone.biconvex.GetHeight() / 2 );
            stone.rigidBody.orientation = quat4f(1,0,0,0);
            stone.rigidBody.linearMomentum = vec3f(0,0,0);
            stone.rigidBody.angularMomentum = vec3f(0,0,0);
            stone.rigidBody.Activate();
            _stones.push_back( stone );
        }
    }

#else

    // IMPORTANT: place initial stone
    stone.rigidBody.position = vec3f( 0, 0, _board.GetThickness() + stone.biconvex.GetHeight()/2 );
    stone.rigidBody.orientation = quat4f(1,0,0,0);
    stone.rigidBody.linearMomentum = vec3f(0,0,0);
    stone.rigidBody.angularMomentum = vec3f(0,0,0);
    stone.rigidBody.Activate();
    _stones.push_back( stone );

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

- (void)setupNotifications
{
    [ [NSNotificationCenter defaultCenter] addObserver : self
                                              selector : @selector(didBecomeActive:)
                                                  name : UIApplicationDidBecomeActiveNotification object:nil ];

    [ [NSNotificationCenter defaultCenter] addObserver : self
                                              selector : @selector(willResignActive:)
                                                  name : UIApplicationWillResignActiveNotification object:nil ];
    
    [ [NSNotificationCenter defaultCenter] addObserver : self
                                              selector : @selector(didEnterBackground:)
                                                  name : UIApplicationDidEnterBackgroundNotification object:nil ];

    [ [NSNotificationCenter defaultCenter] addObserver : self
                                              selector : @selector(willEnterForeground:)
                                                  name : UIApplicationWillEnterForegroundNotification object:nil ];
    
    [ [NSNotificationCenter defaultCenter] addObserver : self
                                              selector : @selector(deviceOrientationDidChange:)
                                                  name : UIDeviceOrientationDidChangeNotification object:nil ];
}

- (void)didBecomeActive:(NSNotification *)notification
{
//    NSLog( @"did become active" );
    _paused = false;
    _hasRendered = false;
}

- (void)willResignActive:(NSNotification *)notification
{
//    NSLog( @"will resign active" );
    _paused = true;
}

- (void)didEnterBackground:(NSNotification *)notification
{
//    NSLog( @"did enter background" );
}

- (void)willEnterForeground:(NSNotification *)notification
{
//    NSLog( @"will enter foreground" );
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

- (void) generateVBAndIBFromMesh:(Mesh<Vertex>&) mesh
                                  vertexBuffer:(GLuint&) vb
                                   indexBuffer:(GLuint&) ib
{
    void * vertexData = mesh.GetVertexBuffer();
    
    assert( vertexData );
    assert( mesh.GetNumIndices() > 0 );
    assert( mesh.GetNumVertices() > 0 );

    glGenBuffers( 1, &vb );
    glBindBuffer( GL_ARRAY_BUFFER, vb );
    glBufferData( GL_ARRAY_BUFFER, sizeof(Vertex) * mesh.GetNumVertices(), vertexData, GL_STATIC_DRAW );
    
    glGenBuffers( 1, &ib );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ib );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, mesh.GetNumIndices()*sizeof(GLushort), mesh.GetIndexBuffer(), GL_STATIC_DRAW );
}

- (void) generateVBAndIBFromTexturedMesh:(Mesh<TexturedVertex>&) mesh
                                          vertexBuffer:(GLuint&) vb
                                           indexBuffer:(GLuint&) ib
{
    void * vertexData = mesh.GetVertexBuffer();
    
    assert( vertexData );
    assert( mesh.GetNumIndices() > 0 );
    assert( mesh.GetNumVertices() > 0 );

    glGenBuffers( 1, &vb );
    glBindBuffer( GL_ARRAY_BUFFER, vb );
    glBufferData( GL_ARRAY_BUFFER, sizeof(TexturedVertex) * mesh.GetNumVertices(), vertexData, GL_STATIC_DRAW );
    
    glGenBuffers( 1, &ib );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ib );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, mesh.GetNumIndices()*sizeof(GLushort), mesh.GetIndexBuffer(), GL_STATIC_DRAW );
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
        
    glEnable( GL_DEPTH_TEST );

    glDepthFunc( GL_LESS );

    glEnable( GL_CULL_FACE );

    // stone shader, textures, VBs/IBs etc.
    {    
        _stoneProgram = [self loadShader:@"StoneShader"];

        [self generateVBAndIBFromMesh:_stoneMesh vertexBuffer:_stoneVertexBuffer indexBuffer:_stoneIndexBuffer];

        _stoneUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _stoneProgram, "normalMatrix" );
        _stoneUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _stoneProgram, "modelViewProjectionMatrix" );
        _stoneUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _stoneProgram, "lightPosition" );
    }
    
    // board shader, textures, VBs/IBs etc.
    {
        _boardProgram = [self loadShader:@"BoardShader"];

        [self generateVBAndIBFromTexturedMesh:_boardMesh vertexBuffer:_boardVertexBuffer indexBuffer:_boardIndexBuffer];
        
        _boardTexture = [self loadTexture:@"wood.jpg"];

        _boardUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _boardProgram, "normalMatrix" );
        _boardUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _boardProgram, "modelViewProjectionMatrix" );
        _boardUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _boardProgram, "lightPosition" );
    }

    // shadow shader, uniforms etc.
    {
        _shadowProgram = [self loadShader:@"ShadowShader"];

        _shadowUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _shadowProgram, "normalMatrix" );
        _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _shadowProgram, "modelViewProjectionMatrix" );
        _shadowUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _shadowProgram, "lightPosition" );
        _shadowUniforms[UNIFORM_ALPHA] = glGetUniformLocation( _shadowProgram, "alpha" );
    }
  
    // grid shader, textures, VBs/IBs etc.
    {
        _gridProgram = [self loadShader:@"GridShader"];

        [self generateVBAndIBFromTexturedMesh:_gridMesh vertexBuffer:_gridVertexBuffer indexBuffer:_gridIndexBuffer];
        
        _gridUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _gridProgram, "normalMatrix" );
        _gridUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _gridProgram, "modelViewProjectionMatrix" );
        _gridUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _gridProgram, "lightPosition" );

        _lineTexture = [self loadTexture:@"line.jpg"];
    }

    // star points
    {
        _pointProgram = [self loadShader:@"PointShader"];

        [self generateVBAndIBFromTexturedMesh:_pointMesh vertexBuffer:_pointVertexBuffer indexBuffer:_pointIndexBuffer];

        _pointUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _pointProgram, "normalMatrix" );
        _pointUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _pointProgram, "modelViewProjectionMatrix" );
        _pointUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _pointProgram, "lightPosition" );

        _pointTexture = [self loadTexture:@"point.jpg"];
    }

    // floor shader, textures, VBs/IBs etc.
    {
        _floorProgram = [self loadShader:@"FloorShader"];

        [self generateVBAndIBFromTexturedMesh:_floorMesh vertexBuffer:_floorVertexBuffer indexBuffer:_floorIndexBuffer];
        
        _floorTexture = [self loadTexture:@"floor.jpg"];

        _floorUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _floorProgram, "normalMatrix" );
        _floorUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _floorProgram, "modelViewProjectionMatrix" );
        _floorUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _floorProgram, "lightPosition" );
    }

    // configure opengl view

    self.preferredFramesPerSecond = 60;

    self.view.multipleTouchEnabled = YES;
}

- (void)destroyBuffer:(GLuint&)buffer
{
    if ( buffer )
    {
        glDeleteBuffers( 1, &buffer );
        buffer = 0;
    }
}

- (void)destroyProgram:(GLuint&)program
{
    if ( program )
    {
        glDeleteBuffers( 1, &program );
        program = 0;
    }
}

- (void)destroyTexture:(GLuint&)texture
{
    if ( texture )
    {
        glDeleteTextures( 1, &texture );
        texture = 0;
    }
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];

    [self destroyBuffer:_stoneIndexBuffer];
    [self destroyBuffer:_stoneVertexBuffer];
    [self destroyProgram:_stoneProgram];

    [self destroyBuffer:_boardIndexBuffer];
    [self destroyBuffer:_boardVertexBuffer];
    [self destroyProgram:_boardProgram];
    [self destroyTexture:_boardTexture];

    [self destroyProgram:_shadowProgram];
    
    // todo: lineTexture should become "grid texture"
    // todo: can combine "point" texture and grid texture and render it all in one go, eg.
    // divide grid texture into four squares. uv coords would need some management, but can work
    
    [self destroyProgram:_gridProgram];
    [self destroyTexture:_lineTexture];
    
    [self destroyProgram:_pointProgram];
    [self destroyTexture:_pointTexture];

    [self destroyBuffer:_floorIndexBuffer];
    [self destroyBuffer:_floorVertexBuffer];
    [self destroyProgram:_floorProgram];
    [self destroyTexture:_floorTexture];
}

- (GLuint)loadTexture:(NSString *)filename
{
    // todo: really should generate texture mipchain using an offline tool
    // then just load that already baked out texture here. this increases load time
    // and will not scale when lots of textures need to be loaded for different boards
    // and different sets of stones
    
    CGImageRef textureImage = [UIImage imageNamed:filename].CGImage;
    if ( !textureImage )
    {
        NSLog( @"Failed to load image %@", filename );
        exit(1);
    }
 
    size_t textureWidth = CGImageGetWidth( textureImage );
    size_t textureHeight = CGImageGetHeight( textureImage );
 
    GLubyte * textureData = (GLubyte *) calloc( textureWidth*textureHeight*4, sizeof(GLubyte) );
 
    CGContextRef textureContext = CGBitmapContextCreate( textureData, textureWidth, textureHeight, 8, textureWidth*4,
        CGImageGetColorSpace(textureImage), kCGImageAlphaPremultipliedLast );
 
    CGContextDrawImage( textureContext, CGRectMake( 0, 0, textureWidth, textureHeight ), textureImage );
 
    CGContextRelease( textureContext );
 
    GLuint textureId;

    glGenTextures( 1, &textureId );
    glBindTexture( GL_TEXTURE_2D, textureId );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData );

    glHint( GL_GENERATE_MIPMAP_HINT, GL_NICEST );
    glGenerateMipmap( GL_TEXTURE_2D );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    
    free( textureData );

    return textureId;
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
    [self incrementCounter:COUNTER_ToggleLocked];
    _locked = !_locked;
    NSLog( @"toggle locked: %s", _locked ? "true" : "false " );
}

- (void)updateTouch:(float)dt
{
    if ( _selected )
    {   
#ifndef MULTIPLE_STONES

        Stone & stone = _stones[0];

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
            for ( int i = 0; i < _stones.size(); ++i )
                _stones[i].rigidBody.angularMomentum *= HoldDamping;
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
                if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), _board.GetThickness() + _biconvex.GetHeight()/2, t ) )
                {
                    const float place_dt = [touch timestamp] - _firstPlacementTimestamp;

                    if ( place_dt < PlaceStoneHardThreshold )
                        [self incrementCounter:COUNTER_PlacedStoneHard];
                    else
                        [self incrementCounter:COUNTER_PlacedStone];

#ifdef MULTIPLE_STONES

                    Stone stone;
                    stone.Initialize( STONE_SIZE_40 );
                    stone.rigidBody.position = rayStart + rayDirection * t;
                    stone.rigidBody.Activate();
                    _stones.push_back( stone );

#else

                    Stone & stone = _stones[0];

                    stone.rigidBody.position = rayStart + rayDirection * t;
                    stone.rigidBody.orientation = quat4f(0,0,0,1);
                    stone.rigidBody.linearMomentum = vec3f(0,0,0);
                    stone.rigidBody.angularMomentum = vec3f(0,0,0);
                    stone.rigidBody.Activate();

                    // IMPORTANT: go into select mode post placement so you can immediately drag the stone
        
                    RigidBodyTransform transform;
                    stone.rigidBody.GetTransform( transform );
                    
                    vec3f intersectionPoint, intersectionNormal;
                    
                    if ( IntersectRayStone( stone.biconvex, transform, rayStart, rayDirection, intersectionPoint, intersectionNormal ) > 0 )
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
        Stone & stone = _stones[0];

        CGPoint selectPoint = [touch locationInView:self.view];

        // IMPORTANT: convert points to pixels!
        const float contentScaleFactor = [self.view contentScaleFactor];
        vec3f point( selectPoint.x, selectPoint.y, 0 );
        point *= contentScaleFactor; 

        vec3f rayStart, rayDirection;
        
        mat4f inverseClipMatrix;
        inverseClipMatrix.load( _inverseClipMatrix.m );
        
        GetPickRay( inverseClipMatrix, point.x(), point.y(), rayStart, rayDirection );
        
        RigidBodyTransform transform;
        stone.rigidBody.GetTransform( transform );

        vec3f intersectionPoint, intersectionNormal;

        if ( IntersectRayStone( stone.biconvex, transform, rayStart, rayDirection, intersectionPoint, intersectionNormal ) > 0 )
        {
//            NSLog( @"select" );
            
            [self incrementCounter:COUNTER_SelectedStone];

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
        Stone & stone = _stones[0];

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

            RigidBodyTransform transform;
            stone.rigidBody.GetTransform( transform );
            vec3f intersectionPoint, intersectionNormal;
            if ( IntersectRayStone( stone.biconvex, transform, rayStart, rayDirection, intersectionPoint, intersectionNormal ) > 0 )
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
                NSLog( @"hold moved" );
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
    if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), _board.GetThickness(), t ) )
    {
        vec3f intersectionPoint = rayStart + rayDirection * t;

        const float x = intersectionPoint.x();
        const float y = intersectionPoint.y();

        float bx, by;
        _board.GetBounds( bx, by );

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
            if ( _zoomed )
                [self incrementCounter:COUNTER_ZoomedOut];
            else
                [self incrementCounter:COUNTER_ZoomedIn];
        
            _zoomed = !_zoomed;
        }

        [NSObject cancelPreviousPerformRequestsWithTarget:self];
    }

#ifndef MULTIPLE_STONES

    if ( _selected )
    {
        Stone & stone = _stones[0];

        if ( [touches containsObject:_selectTouch] )
        {
            UITouch * touch = _selectTouch;

            CGPoint selectPoint = [touch locationInView:self.view];

            const bool onBoard = [self isPointOnBoard:selectPoint];

            const float dx = selectPoint.x - _selectPoint.x;
            const float dy = selectPoint.y - _selectPoint.y;

            const float distance = sqrt( dx*dx + dy*dy );

            const float speed = length( stone.rigidBody.linearVelocity );

            if ( _locked && onBoard && speed > LockedDragSpeedMax )
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
                    [self incrementCounter:COUNTER_FlickedStone];    
                else
                    [self incrementCounter:COUNTER_DraggedStone];
            }
            else
            {
                const float currentTimestamp = [touch timestamp];
                if ( currentTimestamp - _selectTimestamp < 0.1f )
                    [self incrementCounter:COUNTER_TouchedStone];
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
//    [self dropStone];
}

- (void)handleSwipe:(vec3f)delta atPoint:(vec3f)point
{
    #if !LOCKED
    if ( !_locked )
    {
//      NSLog( @"swipe" );

        for ( int i = 0; i < _stones.size(); ++i )
        {
            Stone & stone = _stones[i];

            const vec3f & up = _accelerometer.GetUp();
            stone.rigidBody.angularMomentum += SwipeMomentum * up;
            stone.rigidBody.Activate();
        }

        [self incrementCounter:COUNTER_Swiped];

        _swipedThisFrame = true;
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

- (void)motionBegan:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
    // ...
}

- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
    if ( event.subtype == UIEventSubtypeMotionShake )
    {
//        NSLog( @"shake" );
    }
}

- (void)motionCancelled:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
    // ...
}

- (void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration
{
    _accelerometer.Update( vec3f( acceleration.x, acceleration.y, acceleration.z ) );
}

- (void)deviceOrientationDidChange:(NSNotification *)notification
{
    // ...
}

- (void)incrementCounter:(Counters)counterIndex
{
    if ( ++_counters[counterIndex] == 1 )
        [TestFlight passCheckpoint:[NSString stringWithUTF8String:CounterNames[counterIndex]]];
}

- (vec3f)getDown
{
    return _locked ? vec3f(0,0,-1) : _accelerometer.GetDown();
}

- (vec3f)getGravity
{
    return 9.8f * 10 * [self getDown];
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

    const vec3f & jerkAcceleration = _accelerometer.GetJerkAcceleration();
    const float jerk = length( jerkAcceleration );
    if ( jerk > JerkThreshold )
    {
        for ( int i = 0; i < _stones.size(); ++i )
        {
            Stone & stone = _stones[i];
            stone.rigidBody.ApplyImpulse( JerkScale * jerkAcceleration * stone.rigidBody.mass );
        }
    }

    // detect when the user has asked to launch the stone into the air

    const vec3f down = [self getDown];
    const vec3f up = -down;

    if ( !_locked )
    {
        if ( iPad() )
        {
            // for pads, let the launch go up in the air only!
            // other launches feel wrong and cause nausea -- treat the tablet like the board
            
            const float jerkUp = dot( jerkAcceleration, up );
            
            if ( jerkUp > LaunchThreshold )
            {
                for ( int i = 0; i < _stones.size(); ++i )
                {
                    Stone & stone = _stones[i];
                    stone.rigidBody.ApplyImpulse( jerkUp * LaunchMomentum * up );
                }

                [self incrementCounter:COUNTER_AppliedImpulse];
            }
        }
        else
        {
            // for the iphone let the player shake the stone around like a toy
            // the go stone is trapped inside the phone. this looks cool!
            
            if ( jerk > LaunchThreshold )
            {
                for ( int i = 0; i < _stones.size(); ++i )
                {
                    Stone & stone = _stones[i];
                    stone.rigidBody.ApplyImpulse( jerkAcceleration * LaunchMomentum );
                }

                [self incrementCounter:COUNTER_AppliedImpulse];
            }
        }
    }
    
    // update stone physics

    _collisionPlanes = 0;
    
    #if MULTIPLE_STONES
    const int iterations = 1;
    #else
    const int iterations = 10;
    #endif

    const float iteration_dt = dt / iterations;

    vec3f gravity = [self getGravity];

    for ( int i = 0; i < iterations; ++i )
    {
        for ( int j = 0; j < _stones.size(); ++j )
        {
            Stone & stone = _stones[j];

            if ( !stone.rigidBody.active )
                continue;

            #if !MULTIPLE_STONES            
            if ( !_selected )
            #endif
                stone.rigidBody.linearMomentum += gravity * stone.rigidBody.mass * iteration_dt;

            stone.rigidBody.UpdateMomentum();

            if ( !_selected )
                stone.rigidBody.position += stone.rigidBody.linearVelocity * iteration_dt;

            const int rotation_substeps = 1;
            const float rotation_substep_dt = iteration_dt / rotation_substeps;
            for ( int j = 0; j < rotation_substeps; ++j )
            {
                quat4f spin;
                AngularVelocityToSpin( stone.rigidBody.orientation, stone.rigidBody.angularVelocity, spin );
                stone.rigidBody.orientation += spin * rotation_substep_dt;
                stone.rigidBody.orientation = normalize( stone.rigidBody.orientation );
            }        
            
            stone.rigidBody.UpdateTransform();

            StaticContact contact;

            bool iteration_collided = false;
            
            const float e = 0.5f;
            const float u = 0.5f;
        
            if ( !_locked )
            {
                // collision between stone and near plane

                vec4f nearPlane( 0, 0, -1, -_smoothZoom );
                
                if ( StonePlaneCollision( stone.biconvex, nearPlane, stone.rigidBody, contact ) )
                {
                    ApplyCollisionImpulseWithFriction( contact, e, u );
                    iteration_collided = true;
                    _collisionPlanes |= COLLISION_NearPlane;
                }

                // collision between stone and left plane

                if ( StonePlaneCollision( stone.biconvex, frustum.left, stone.rigidBody, contact ) )
                {
                    ApplyCollisionImpulseWithFriction( contact, e, u );
                    iteration_collided = true;
                    _collisionPlanes |= COLLISION_LeftPlane;
                }

                // collision between stone and right plane

                if ( StonePlaneCollision( stone.biconvex, frustum.right, stone.rigidBody, contact ) )
                {
                    ApplyCollisionImpulseWithFriction( contact, e, u );
                    iteration_collided = true;
                    _collisionPlanes |= COLLISION_RightPlane;
                }

                // collision between stone and top plane

                if ( StonePlaneCollision( stone.biconvex, frustum.top, stone.rigidBody, contact ) )
                {
                    ApplyCollisionImpulseWithFriction( contact, e, u );
                    iteration_collided = true;
                    _collisionPlanes |= COLLISION_TopPlane;
                }

                // collision between stone and bottom plane

                if ( StonePlaneCollision( stone.biconvex, frustum.bottom, stone.rigidBody, contact ) )
                {
                    ApplyCollisionImpulseWithFriction( contact, e, u );
                    iteration_collided = true;
                    _collisionPlanes |= COLLISION_BottomPlane;
                }
            }

            // collision between stone and ground plane
            
            if ( StonePlaneCollision( stone.biconvex, vec4f(0,0,1,0), stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, e, u );
                iteration_collided = true;
                _collisionPlanes |= COLLISION_GroundPlane;
            }

            // collision between stone and board

            if ( StoneBoardCollision( stone.biconvex, _board, stone.rigidBody, contact, true, _selected ) )
            {
                ApplyCollisionImpulseWithFriction( contact, e, u );
                iteration_collided = true;
                _collisionPlanes |= COLLISION_Board;
            }

            // this is a *massive* hack to approximate rolling/spinning
            // friction and it is completely made up and not accurate at all!

            if ( iteration_collided )
            {
                #if LOCKED

                    const float factor = DecayFactor( 0.9f, iteration_dt );
                    stone.rigidBody.angularMomentum *= factor;

                #else

                    float momentum = length( stone.rigidBody.angularMomentum );
                
                    #if MULTIPLE_STONES
                    const float factor_a = DecayFactor( 0.75f, iteration_dt );
                    const float factor_b = DecayFactor( 0.99f, iteration_dt );
                    #else
                    const float factor_a = 0.9925f;
                    const float factor_b = 0.9995f;
                    #endif
                    
                    const float a = 0.0f;
                    const float b = 1.0f;
                    
                    if ( momentum >= b )
                    {
                        stone.rigidBody.angularMomentum *= factor_b;
                    }
                    else if ( momentum <= a )
                    {
                        stone.rigidBody.angularMomentum *= factor_a;
                    }
                    else
                    {
                        const float alpha = ( momentum - a ) / ( b - a );
                        const float factor = factor_a * ( 1 - alpha ) + factor_b * alpha;
                        stone.rigidBody.angularMomentum *= factor;
                    }

                    collided = true;

                #endif
            }

            // apply damping

            #if MULTIPLE_STONES
            const float linear_factor = 0.99999f;
            const float angular_factor = 0.99999f;
            #else
            const float linear_factor = DecayFactor( 0.999f, iteration_dt );
            const float angular_factor = DecayFactor( 0.999f, iteration_dt );
            #endif

            stone.rigidBody.linearMomentum *= linear_factor;
            stone.rigidBody.angularMomentum *= angular_factor;
        }

        // deactivate stones at rest

        for ( int i = 0; i < _stones.size(); ++i )
        {
            Stone & stone = _stones[i];

            if ( length_squared( stone.rigidBody.linearVelocity ) < DeactivateLinearThresholdSquared &&
                 length_squared( stone.rigidBody.angularVelocity ) < DeactivateAngularThresholdSquared )
            {
                stone.rigidBody.deactivateTimer += dt;
                if ( stone.rigidBody.deactivateTimer >= DeactivateTime )
                    stone.rigidBody.Deactivate();
            }
            else
                stone.rigidBody.deactivateTimer = 0;
        }
    }
}

- (void)updateDetection:(float)dt
{
#ifndef MULTIPLE_STONES

    Stone & stone = _stones[0];

    // update seconds since last swipe

    if ( _swipedThisFrame )
        _secondsSinceLastSwipe = 0;
    else
        _secondsSinceLastSwipe += dt;

    // update seconds since collision

    for ( int i = 0; i < COLLISION_NumValues; ++i )
        _secondsSinceCollision[i] += dt;

    if ( _collisionPlanes == COLLISION_Board )
        _secondsSinceCollision[COLLISION_Board] = 0;

    if ( _collisionPlanes == COLLISION_GroundPlane )
        _secondsSinceCollision[COLLISION_GroundPlane] = 0;

    if ( _collisionPlanes == COLLISION_LeftPlane )
        _secondsSinceCollision[COLLISION_LeftPlane] = 0;

    if ( _collisionPlanes == COLLISION_RightPlane )
        _secondsSinceCollision[COLLISION_RightPlane] = 0;

    if ( _collisionPlanes == COLLISION_TopPlane )
        _secondsSinceCollision[COLLISION_TopPlane] = 0;

    if ( _collisionPlanes == COLLISION_BottomPlane )
        _secondsSinceCollision[COLLISION_BottomPlane] = 0;

    if ( _collisionPlanes == COLLISION_NearPlane )
        _secondsSinceCollision[COLLISION_NearPlane] = 0;

    const bool recentBoardCollision = _secondsSinceCollision[COLLISION_Board] < 0.1f;
    const bool recentGroundCollision = _secondsSinceCollision[COLLISION_GroundPlane] < 0.1f;
    const bool recentLeftCollision = _secondsSinceCollision[COLLISION_LeftPlane] < 0.1f;
    const bool recentRightCollision = _secondsSinceCollision[COLLISION_RightPlane] < 0.1f;
    const bool recentTopCollision = _secondsSinceCollision[COLLISION_TopPlane] < 0.1f;
    const bool recentBottomCollision = _secondsSinceCollision[COLLISION_BottomPlane] < 0.1f;
    const bool recentNearCollision = _secondsSinceCollision[COLLISION_NearPlane] < 0.1f;

    // detect at rest on each collision plane and board

    const bool atRest = length( stone.rigidBody.linearVelocity ) < 1.0f &&
                        length( stone.rigidBody.angularVelocity ) < 0.5f;

    if ( _detectionTimer[CONDITION_AtRestBoard].Update( dt, atRest && _collisionPlanes == COLLISION_Board ) )
    {
        [self incrementCounter:COUNTER_AtRestBoard];
    }

    if ( _detectionTimer[CONDITION_AtRestGroundPlane].Update( dt, atRest && _collisionPlanes == COLLISION_GroundPlane ) )
    {
        [self incrementCounter:COUNTER_AtRestGroundPlane];
    }

    if ( _detectionTimer[CONDITION_AtRestLeftPlane].Update( dt, atRest && _collisionPlanes == COLLISION_LeftPlane ) )
    {
        [self incrementCounter:COUNTER_AtRestLeftPlane];
    }

    if ( _detectionTimer[CONDITION_AtRestRightPlane].Update( dt, atRest && _collisionPlanes == COLLISION_RightPlane ) )
    {
        [self incrementCounter:COUNTER_AtRestRightPlane];
    }

    if ( _detectionTimer[CONDITION_AtRestTopPlane].Update( dt, atRest && _collisionPlanes == COLLISION_TopPlane ) )
    {
        [self incrementCounter:COUNTER_AtRestTopPlane];
    }

    if ( _detectionTimer[CONDITION_AtRestBottomPlane].Update( dt, atRest && _collisionPlanes == COLLISION_BottomPlane ) )
    {
        [self incrementCounter:COUNTER_AtRestBottomPlane];
    }

    if ( _detectionTimer[CONDITION_AtRestNearPlane].Update( dt, atRest && _collisionPlanes == COLLISION_NearPlane ) )
    {
        [self incrementCounter:COUNTER_AtRestNearPlane];
    }

    // detect sliding on each collision plane

    const float SlideTime = 0.5f;

    const bool sliding = !atRest &&
                         length( stone.rigidBody.linearVelocity ) < 10.0f &&
                         length( stone.rigidBody.angularVelocity ) < 1.0f;

    if ( _detectionTimer[CONDITION_SlidingBoard].Update( dt, sliding && _collisionPlanes == COLLISION_Board, SlideTime ) )
    {
        [self incrementCounter:COUNTER_SlidingBoard];
    }

    if ( _detectionTimer[CONDITION_SlidingGroundPlane].Update( dt, sliding && _collisionPlanes == COLLISION_GroundPlane, SlideTime ) )
    {
        [self incrementCounter:COUNTER_SlidingGroundPlane];
    }

    if ( _detectionTimer[CONDITION_SlidingLeftPlane].Update( dt, sliding && _collisionPlanes == COLLISION_LeftPlane, SlideTime ) )
    {
        [self incrementCounter:COUNTER_SlidingLeftPlane];
    }

    if ( _detectionTimer[CONDITION_SlidingRightPlane].Update( dt, sliding && _collisionPlanes == COLLISION_RightPlane, SlideTime ) )
    {
        [self incrementCounter:COUNTER_SlidingRightPlane];
    }

    if ( _detectionTimer[CONDITION_SlidingTopPlane].Update( dt, sliding && _collisionPlanes == COLLISION_TopPlane, SlideTime ) )
    {
        [self incrementCounter:COUNTER_SlidingTopPlane];
    }

    if ( _detectionTimer[CONDITION_SlidingBottomPlane].Update( dt, sliding && _collisionPlanes == COLLISION_BottomPlane, SlideTime ) )
    {
        [self incrementCounter:COUNTER_SlidingBottomPlane];
    }

    if ( _detectionTimer[CONDITION_SlidingNearPlane].Update( dt, sliding && _collisionPlanes == COLLISION_NearPlane, SlideTime ) )
    {
        [self incrementCounter:COUNTER_SlidingNearPlane];
    }

    // detect spinning on each collision plane

    if ( !_locked )
    {
        const float SpinTime = 0.25f;

        const vec3f up = - [self getDown];

        RigidBodyTransform rigidBodyTransform;
        stone.rigidBody.GetTransform( rigidBodyTransform );
        
        const float upSpin = fabs( dot( stone.rigidBody.angularVelocity, up ) );
        const float totalSpin = length( stone.rigidBody.angularVelocity );

        const bool spinning = length( stone.rigidBody.linearVelocity ) < 10.0 &&
                              dot( rigidBodyTransform.GetUp(), up ) < 0.25f &&
                              upSpin > 1.0f && fabs( upSpin / totalSpin ) > 0.7f &&
                              _secondsSinceLastSwipe < 1.0f;

        if ( _detectionTimer[CONDITION_SpinningBoard].Update( dt, spinning && recentBoardCollision, SpinTime ) )
        {
            [self incrementCounter:COUNTER_SpinningBoard];
        }

        if ( _detectionTimer[CONDITION_SpinningGroundPlane].Update( dt, spinning && recentGroundCollision, SpinTime ) )
        {
            [self incrementCounter:COUNTER_SpinningGroundPlane];
        }

        if ( _detectionTimer[CONDITION_SpinningLeftPlane].Update( dt, spinning && recentLeftCollision, SpinTime ) )
        {
            [self incrementCounter:COUNTER_SpinningLeftPlane];
        }

        if ( _detectionTimer[CONDITION_SpinningRightPlane].Update( dt, spinning && recentRightCollision, SpinTime ) )
        {
            [self incrementCounter:COUNTER_SpinningRightPlane];
        }

        if ( _detectionTimer[CONDITION_SpinningTopPlane].Update( dt, spinning && recentTopCollision, SpinTime ) )
        {
            [self incrementCounter:COUNTER_SpinningTopPlane];
        }

        if ( _detectionTimer[CONDITION_SpinningBottomPlane].Update( dt, spinning && recentBottomCollision, SpinTime ) )
        {
            [self incrementCounter:COUNTER_SpinningBottomPlane];
        }

        if ( _detectionTimer[CONDITION_SpinningNearPlane].Update( dt, spinning && recentNearCollision, SpinTime ) )
        {
            [self incrementCounter:COUNTER_SpinningNearPlane];
        }
    }

    // update orientation detection

    if ( !_locked )
    {
        const float OrientationTime = 1.0f;

        const vec3f up = - [self getDown];

        if ( _detectionTimer[CONDITION_OrientationPerfectlyFlat].Update( dt, dot( up, vec3f(0,0,1) ) > 0.99f, OrientationTime ) )
        {
            [self incrementCounter:COUNTER_OrientationPerfectlyFlat];
        }
        
        if ( _detectionTimer[CONDITION_OrientationNeutral].Update( dt, dot( up, vec3f(0,0,1) ) > 0.75f, OrientationTime ) )
        {
            [self incrementCounter:COUNTER_OrientationNeutral];
        }

        if ( _detectionTimer[CONDITION_OrientationLeft].Update( dt, dot( up, vec3f(1,0,0) ) > 0.75f, OrientationTime ) )
        {
            [self incrementCounter:COUNTER_OrientationLeft];
        }

        if ( _detectionTimer[CONDITION_OrientationRight].Update( dt, dot( up, vec3f(-1,0,0) ) > 0.75f, OrientationTime ) )
        {
            [self incrementCounter:COUNTER_OrientationRight];
        }

        if ( _detectionTimer[CONDITION_OrientationUp].Update( dt, dot( up, vec3f(0,1,0) ) > 0.75f, OrientationTime ) )
        {
            [self incrementCounter:COUNTER_OrientationUp];
        }
        
        if ( _detectionTimer[CONDITION_OrientationDown].Update( dt, dot( up, vec3f(0,-1,0) ) > 0.75f, OrientationTime ) )
        {
            [self incrementCounter:COUNTER_OrientationDown];
        }

        if ( _detectionTimer[CONDITION_OrientationUpsideDown].Update( dt, dot( up, vec3f(0,0,-1) ) > 0.75f, OrientationTime ) )
        {
            [self incrementCounter:COUNTER_OrientationUpsideDown];
        }
    }

    // clear values this frame

    _swipedThisFrame = false;

#endif
}

- (void)updateLights
{
    _lightPosition = vec3f( 30, 30, 100 );
}

- (float)getTargetZoom
{
    return iPad() ? ( _zoomed ? ZoomIn_iPad : ZoomOut_iPad ) : ( _zoomed ? ZoomIn_iPhone : ZoomOut_iPhone );
}

- (void)render
{
    float aspect = fabsf( self.view.bounds.size.width / self.view.bounds.size.height );
    
    const float targetZoom = [self getTargetZoom];
    
    _smoothZoom += ( targetZoom - _smoothZoom ) * ( _zoomed ? ZoomInTightness : ZoomOutTightness );
    
    GLKMatrix4 projectionMatrix = GLKMatrix4MakePerspective( GLKMathDegreesToRadians(40.0f), aspect, 0.1f, 100.0f );
    
    GLKMatrix4 baseModelViewMatrix = GLKMatrix4MakeLookAt( 0, 0, _smoothZoom,
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
    
    if ( _paused )
        return;

    // render floor

    {
        glUseProgram( _floorProgram );
                
        glBindTexture( GL_TEXTURE_2D, _floorTexture );

        glBindBuffer( GL_ARRAY_BUFFER, _floorVertexBuffer );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _floorIndexBuffer );
        
        glEnableVertexAttribArray( GLKVertexAttribPosition );
        glEnableVertexAttribArray( GLKVertexAttribNormal );
        glEnableVertexAttribArray( GLKVertexAttribTexCoord0 );

        glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), 0 );
        glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)16 );
        glVertexAttribPointer( GLKVertexAttribTexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)32 );

        glUniformMatrix4fv( _floorUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _floorModelViewProjectionMatrix.m );
        glUniformMatrix3fv( _floorUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _floorNormalMatrix.m );
        glUniform3fv( _floorUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&_lightPosition );

        glDrawElements( GL_TRIANGLES, _floorMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );

        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

        glDisableVertexAttribArray( GLKVertexAttribPosition );
        glDisableVertexAttribArray( GLKVertexAttribNormal );
        glDisableVertexAttribArray( GLKVertexAttribTexCoord0 );
    }

    // render board

    {
        glUseProgram( _boardProgram );
                
        glBindTexture( GL_TEXTURE_2D, _boardTexture );

        glBindBuffer( GL_ARRAY_BUFFER, _boardVertexBuffer );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _boardIndexBuffer );
        
        glEnableVertexAttribArray( GLKVertexAttribPosition );
        glEnableVertexAttribArray( GLKVertexAttribNormal );
        glEnableVertexAttribArray( GLKVertexAttribTexCoord0 );

        glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), 0 );
        glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)16 );
        glVertexAttribPointer( GLKVertexAttribTexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)32 );

        glUniformMatrix4fv( _boardUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _boardModelViewProjectionMatrix.m );
        glUniformMatrix3fv( _boardUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _boardNormalMatrix.m );
        glUniform3fv( _boardUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&_lightPosition );

        glDrawElements( GL_TRIANGLES, _boardMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );

        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

        glDisableVertexAttribArray( GLKVertexAttribPosition );
        glDisableVertexAttribArray( GLKVertexAttribNormal );
        glDisableVertexAttribArray( GLKVertexAttribTexCoord0 );
    }

    // render grid
    
    {
        glUseProgram( _gridProgram );
        
        glBindTexture( GL_TEXTURE_2D, _lineTexture );

        glBindBuffer( GL_ARRAY_BUFFER, _gridVertexBuffer );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _gridIndexBuffer );
        
        glEnableVertexAttribArray( GLKVertexAttribPosition );
        glEnableVertexAttribArray( GLKVertexAttribNormal );
        glEnableVertexAttribArray( GLKVertexAttribTexCoord0 );

        glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), 0 );
        glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)16 );
        glVertexAttribPointer( GLKVertexAttribTexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)32 );
        
        glUniformMatrix4fv( _gridUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _gridModelViewProjectionMatrix.m );
        glUniformMatrix3fv( _gridUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _gridNormalMatrix.m );
        glUniform3fv( _gridUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&_lightPosition );
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_ZERO, GL_SRC_COLOR );

        glDisable( GL_DEPTH_TEST );

        glDrawElements( GL_TRIANGLES, _gridMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        
        glEnable( GL_DEPTH_TEST );

        glDisable( GL_BLEND );

        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        
        glDisableVertexAttribArray( GLKVertexAttribPosition );
        glDisableVertexAttribArray( GLKVertexAttribNormal );
        glDisableVertexAttribArray( GLKVertexAttribTexCoord0 );
    }

    // render star points
    
    {
        glUseProgram( _pointProgram );
        
        glBindTexture( GL_TEXTURE_2D, _pointTexture );

        glBindBuffer( GL_ARRAY_BUFFER, _pointVertexBuffer );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _pointIndexBuffer );
        
        glEnableVertexAttribArray( GLKVertexAttribPosition );
        glEnableVertexAttribArray( GLKVertexAttribNormal );
        glEnableVertexAttribArray( GLKVertexAttribTexCoord0 );

        glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), 0 );
        glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)16 );
        glVertexAttribPointer( GLKVertexAttribTexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)32 );
        
        glUniformMatrix4fv( _pointUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _pointModelViewProjectionMatrix.m );
        glUniformMatrix3fv( _pointUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _pointNormalMatrix.m );
        glUniform3fv( _pointUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&_lightPosition );
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_ZERO, GL_SRC_COLOR );

        glDisable( GL_DEPTH_TEST );

        glDrawElements( GL_TRIANGLES, _pointMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        
        glEnable( GL_DEPTH_TEST );

        glDisable( GL_BLEND );

        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        
        glDisableVertexAttribArray( GLKVertexAttribPosition );
        glDisableVertexAttribArray( GLKVertexAttribNormal );
        glDisableVertexAttribArray( GLKVertexAttribTexCoord0 );
    }
    
    // render board shadow on ground
    
    {
        glUseProgram( _shadowProgram );
                
        glBindBuffer( GL_ARRAY_BUFFER, _boardVertexBuffer );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _boardIndexBuffer );
        
        glEnableVertexAttribArray( GLKVertexAttribPosition );
        glEnableVertexAttribArray( GLKVertexAttribNormal );

        glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), 0 );
        glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)16 );

        float boardShadowAlpha = 1.0f;

        GLKMatrix4 view = baseModelViewMatrix;

        mat4f shadow_matrix;
        MakeShadowMatrix( vec4f(0,0,1,-0.1f), vec4f( _lightPosition.x(), _lightPosition.y(), _lightPosition.z() * 0.5f, 0 ), shadow_matrix );
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

        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

        glDisableVertexAttribArray( GLKVertexAttribPosition );
        glDisableVertexAttribArray( GLKVertexAttribNormal );
    }

    // render stone shadow on ground
    
    {
        glUseProgram( _shadowProgram );
        
        glBindBuffer( GL_ARRAY_BUFFER, _stoneVertexBuffer );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _stoneIndexBuffer );
        
        glEnableVertexAttribArray( GLKVertexAttribPosition );
        glEnableVertexAttribArray( GLKVertexAttribNormal );
        
        glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0 );
        glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)16 );
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        for ( int i = 0; i < _stones.size(); ++i )
        {
            Stone & stone = _stones[i];
            
            GLKMatrix4 view = baseModelViewMatrix;
            
            float opengl_transform[16];
            stone.rigidBody.transform.localToWorld.store( opengl_transform );

            GLKMatrix4 model = GLKMatrix4MakeWithArray( opengl_transform );
            
            mat4f shadow_matrix;
            MakeShadowMatrix( vec4f(0,0,1,0), vec4f( _lightPosition.x(), _lightPosition.y(), _lightPosition.z(), 1 ), shadow_matrix );
            float shadow_data[16];
            shadow_matrix.store( shadow_data );
            GLKMatrix4 shadow = GLKMatrix4MakeWithArray( shadow_data );

            GLKMatrix4 modelView = GLKMatrix4Multiply( view, GLKMatrix4Multiply( shadow, model ) );

            GLKMatrix4 stoneGroundShadowModelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, modelView );

            glUniformMatrix4fv( _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, stoneGroundShadowModelViewProjectionMatrix.m );

            float shadowAlpha = GetShadowAlpha( stone );

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
        
        glBindBuffer( GL_ARRAY_BUFFER, _stoneVertexBuffer );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _stoneIndexBuffer );
        
        glEnableVertexAttribArray( GLKVertexAttribPosition );
        glEnableVertexAttribArray( GLKVertexAttribNormal );
        
        glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0 );
        glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)16 );
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_GREATER );

        for ( int i = 0; i < _stones.size(); ++i )
        {
            Stone & stone = _stones[i];

            if ( stone.rigidBody.position.z() < _board.GetThickness() )
                continue;
        
            float shadowAlpha = GetShadowAlpha( stone );
            
            if ( shadowAlpha == 0.0f )
                continue;

            GLKMatrix4 view = baseModelViewMatrix;
            
            float opengl_transform[16];
            stone.rigidBody.transform.localToWorld.store( opengl_transform );

            GLKMatrix4 model = GLKMatrix4MakeWithArray( opengl_transform );
            
            // HACK: the fact that I have to minus here indicates
            // something wrong in the shadow matrix derivation. perhaps it assumes left handed?
            mat4f shadow_matrix;
            MakeShadowMatrix( vec4f(0,0,1,-_board.GetThickness()+0.1f), vec4f( _lightPosition.x(), _lightPosition.y(), _lightPosition.z(), 1 ), shadow_matrix );
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
        
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        
        glDisableVertexAttribArray( GLKVertexAttribPosition );
        glDisableVertexAttribArray( GLKVertexAttribNormal );
    }

    // render stone

    {
        glUseProgram( _stoneProgram );
                
        glBindBuffer( GL_ARRAY_BUFFER, _stoneVertexBuffer );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _stoneIndexBuffer );
        
        glEnableVertexAttribArray( GLKVertexAttribPosition );
        glEnableVertexAttribArray( GLKVertexAttribNormal );

        glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0 );
        glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)16 );

        for ( int i = 0; i < _stones.size(); ++i )
        {
            Stone & stone = _stones[i];
            
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
            glUniform3fv( _stoneUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&_lightPosition );

            glDrawElements( GL_TRIANGLES, _stoneMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        }

        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

        glDisableVertexAttribArray( GLKVertexAttribPosition );
        glDisableVertexAttribArray( GLKVertexAttribNormal );
    }

    const GLenum discards[]  = { GL_DEPTH_ATTACHMENT };
    glDiscardFramebufferEXT( GL_FRAMEBUFFER, 1, discards );
    
    _hasRendered = true;
}

- (void)update
{
    float dt = self.timeSinceLastUpdate;

    if ( dt > 1 / 10.0f )
        dt = 1 / 10.0f;
    
    if ( _paused )
        dt = 0.0f;

    [self updateTouch:dt];

    [self updatePhysics:dt];

    [self updateDetection:dt];

    [self updateLights];

    [self render];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    [self render];
}

#pragma mark -  OpenGL ES 2 shader compilation

- (GLuint)loadShader:(NSString*)filename
{
    GLuint vertShader, fragShader;

    NSString *vertShaderPathname, *fragShaderPathname;
    
    // Create shader program.
    GLuint program = glCreateProgram();
    
    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:filename ofType:@"vertex"];
    
    if ( !vertShaderPathname )
    {
        NSLog( @"Could not find vertex shader: %@", filename );
        return 0;
    }

    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname])
    {
        NSLog( @"Failed to compile vertex shader" );
        return 0;
    }
    
    // Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:filename ofType:@"fragment"];

    if ( !fragShaderPathname )
    {
        NSLog( @"Could not find fragment shader: %@", filename );
        return 0;
    }

    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname])
    {
        NSLog( @"Failed to compile fragment shader" );
        return 0;
    }
    
    // Attach vertex shader to program.
    glAttachShader( program, vertShader );
    
    // Attach fragment shader to program.
    glAttachShader( program, fragShader );
    
    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation( program, GLKVertexAttribPosition, "position" );
    glBindAttribLocation( program, GLKVertexAttribNormal, "normal" );
    glBindAttribLocation( program, GLKVertexAttribTexCoord0, "texCoords" );
    
    // Link program.
    if (![self linkProgram:program])
    {
        NSLog( @"Failed to link program: %d", program );
        
        if ( vertShader )
        {
            glDeleteShader( vertShader );
            vertShader = 0;
        }
        if ( fragShader )
        {
            glDeleteShader( fragShader );
            fragShader = 0;
        }
        if ( program )
        {
            glDeleteProgram( program );
            program = 0;
        }
        
        return 0;
    }
        
    // Release vertex and fragment shaders.
    if ( vertShader )
    {
        glDetachShader( program, vertShader );
        glDeleteShader( vertShader );
    }
    if ( fragShader )
    {
        glDetachShader( program, fragShader );
        glDeleteShader( fragShader );
    }
    
    return program;
}

- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file
{
    GLint status;
    const GLchar *source;
    
    source = (GLchar *)[[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] UTF8String];
    if (!source)
    {
        NSLog( @"Failed to load vertex shader" );
        return NO;
    }
    
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv( *shader, GL_INFO_LOG_LENGTH, &logLength );
    if ( logLength > 0 )
    {
        GLchar *log = (GLchar *)malloc( logLength );
        glGetShaderInfoLog( *shader, logLength, &logLength, log );
        NSLog( @"Shader compile log:\n%s", log );
        free( log );
    }
#endif
    
    glGetShaderiv( *shader, GL_COMPILE_STATUS, &status );
    if ( status == 0 )
    {
        glDeleteShader( *shader );
        return NO;
    }
    
    return YES;
}

- (BOOL)linkProgram:(GLuint)prog
{
    GLint status;
    glLinkProgram( prog );
    
#if defined(DEBUG)
    GLint logLength;
    glGetProgramiv( prog, GL_INFO_LOG_LENGTH, &logLength );
    if ( logLength > 0 )
    {
        GLchar *log = (GLchar *)malloc( logLength );
        glGetProgramInfoLog( prog, logLength, &logLength, log );
        NSLog( @"Program link log:\n%s", log );
        free( log );
    }
#endif
    
    glGetProgramiv( prog, GL_LINK_STATUS, &status );
    if ( status == 0 )
    {
        return NO;
    }
    
    return YES;
}

- (BOOL)validateProgram:(GLuint)prog
{
    GLint logLength, status;
    
    glValidateProgram( prog );
    glGetProgramiv( prog, GL_INFO_LOG_LENGTH, &logLength );
    if ( logLength > 0 )
    {
        GLchar *log = (GLchar *)malloc( logLength );
        glGetProgramInfoLog( prog, logLength, &logLength, log );
        NSLog( @"Program validate log:\n%s", log );
        free(log);
    }
    
    glGetProgramiv( prog, GL_VALIDATE_STATUS, &status );
    if ( status == 0 )
    {
        return NO;
    }
    
    return YES;
}

@end
