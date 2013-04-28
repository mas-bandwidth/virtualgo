//
//  ViewController.m
//  Virtual Go
//
//  Created by Glenn Fiedler on 4/13/13.
//  Copyright (c) 2013 Glenn Fiedler. All rights reserved.
//

#import "ViewController.h"
#import "testflight/TestFlight.h"

#include "Common.h"
#include "Biconvex.h"
#include "Mesh.h"
#include "Stone.h"
#include "Board.h"
#include "CollisionDetection.h"
#include "CollisionResponse.h"

enum
{
    UNIFORM_MODELVIEWPROJECTION_MATRIX,
    UNIFORM_NORMAL_MATRIX,
    UNIFORM_LIGHT_POSITION,
    NUM_UNIFORMS
};

enum Counters
{
    COUNTER_ZoomedIn,
    COUNTER_ZoomedOut,
    COUNTER_AppliedImpulse,
    COUNTER_Swiped,
    COUNTER_SelectedStone,
    COUNTER_DraggedStone,
    COUNTER_FlickedStone,
    COUNTER_TouchedStone,

    COUNTER_HitNearPlane,

    COUNTER_AtRestGroundPlane,
    COUNTER_AtRestLeftPlane,
    COUNTER_AtRestRightPlane,
    COUNTER_AtRestTopPlane,
    COUNTER_AtRestBottomPlane,
    COUNTER_AtRestNearPlane,

    COUNTER_SlidingGroundPlane,
    COUNTER_SlidingLeftPlane,
    COUNTER_SlidingRightPlane,
    COUNTER_SlidingTopPlane,
    COUNTER_SlidingBottomPlane,
    COUNTER_SlidingNearPlane,

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
    "zoomed in",
    "zoomed out",
    "applied impulse",
    "swiped",
    "selected stone",
    "dragged stone",
    "flicked stone",
    "touched stone",
    
    "hit near plane",

    "at rest ground plane",
    "at rest left plane",
    "at rest right plane",
    "at rest top plane",
    "at rest bottom plane",
    "at rest near plane",
    
    "sliding ground plane",
    "sliding left plane",
    "sliding right plane",
    "sliding top plane",
    "sliding bottom plane",
    "sliding near plane",

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

enum Conditions
{
    CONDITION_AtRestGroundPlane,
    CONDITION_AtRestLeftPlane,
    CONDITION_AtRestRightPlane,
    CONDITION_AtRestTopPlane,
    CONDITION_AtRestBottomPlane,
    CONDITION_AtRestNearPlane,

    CONDITION_SlidingGroundPlane,
    CONDITION_SlidingLeftPlane,
    CONDITION_SlidingRightPlane,
    CONDITION_SlidingTopPlane,
    CONDITION_SlidingBottomPlane,
    CONDITION_SlidingNearPlane,

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
    COLLISION_GroundPlane,
    COLLISION_LeftPlane,
    COLLISION_RightPlane,
    COLLISION_TopPlane,
    COLLISION_BottomPlane,
    COLLISION_NearPlane,
    COLLISION_NumValues
};

@interface ViewController ()
{    
    GLKMatrix4 _clipMatrix;
    GLKMatrix4 _inverseClipMatrix;
        
    Stone _stone;
    Mesh<Vertex> _stoneMesh;
    GLuint _stoneProgram;
    GLuint _stoneVertexBuffer;
    GLuint _stoneIndexBuffer;
    GLKMatrix4 _stoneModelViewProjectionMatrix;
    GLKMatrix3 _stoneNormalMatrix;
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
    GLKMatrix4 _shadowModelViewProjectionMatrix;
    GLKMatrix3 _shadowNormalMatrix;
    GLint _shadowUniforms[NUM_UNIFORMS];

    vec3f _lightPosition;

    bool _paused;
    bool _zoomed;
    bool _hasRendered;

    float _smoothZoom;
    
    vec3f _rawAcceleration;                 // raw data from the accelometer
    vec3f _smoothedAcceleration;            // smoothed acceleration = gravity
    vec3f _jerkAcceleration;                // jerk acceleration = short motions, above a threshold = bump the board

    bool _swipeStarted;
    UITouch * _swipeTouch;
    float _swipeTime;
    CGPoint _swipeStartPoint;

    bool _holdStarted;
    UITouch * _holdTouch;
    float _holdTime;
    CGPoint _holdPoint;

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

const float ZoomIn_iPad = 12;               // tuned to real go stone size on iPad 3
const float ZoomOut_iPad = 26;

const float ZoomIn_iPhone = 6;              // tuned to real go stone size on iPhone 5
const float ZoomOut_iPhone = 12;

const float ZoomInTightness = 0.25f;
const float ZoomOutTightness = 0.15f;

const float AccelerometerFrequency = 30;
const float AccelerometerTightness = 0.1f;

const float JerkThreshold = 0.1f;

const float LaunchThreshold = 0.5f;
const float LaunchMomentum = 8;

const float MinimumSwipeLength = 50;            // points
const float SwipeLengthPerSecond = 250;         // points
const float MaxSwipeTime = 1.0f;                // seconds
const float SwipeMomentum = 10.0f;

const float HoldDelay = 0.05f;                  // seconds
const float HoldDamping = 0.75f;                
const float HoldMoveThreshold = 40;             // points

const float SelectDamping = 0.75f;

const float TouchImpulse = 4.0f;

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
    
    _stone.Initialize( STONE_SIZE_40 );

    GenerateBiconvexMesh( _stoneMesh, _stone.biconvex, 4 );

    [self generateBoardMesh];

    [self setupGL];
  
    _paused = true;
    _zoomed = false;
    _hasRendered = false;
    
    _smoothZoom = [self getTargetZoom];
    
    _rawAcceleration = vec3f(0,0,-1);
    _smoothedAcceleration = vec3f(0,0,-1);
    _jerkAcceleration = vec3f(0,0,0);

    _swipeStarted = false;
    _holdStarted = false;
    _selected = false;

    _collisionPlanes = 0;

    memset( _secondsSinceCollision, 0, sizeof( _secondsSinceCollision ) );

    _swipedThisFrame = false;
    _secondsSinceLastSwipe = 0;

    _lightPosition = vec3f( 10, 10, 100 );

    [self dropStone];
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

    // Dispose of any resources that can be recreated.
}

- (void)generateBoardMesh
{
    TexturedVertex a,b,c,d;

    const float s = 10;

    a.position = vec3f( -s, -s, 0 );
    a.normal = vec3f( 0, 0, 1 );
    a.texCoords = vec2f( 0, 0 );

    b.position = vec3f( -s, s, 0 );
    b.normal = vec3f( 0, 0, 1 );
    b.texCoords = vec2f( 0, 1 );

    c.position = vec3f( s, s, 0 );
    c.normal = vec3f( 0, 0, 1 );
    c.texCoords = vec2f( 1, 1 );

    d.position = vec3f( s, -s, 0 );
    d.normal = vec3f( 0, 0, 1 );
    d.texCoords = vec2f( 1, 0 );

    _boardMesh.AddTriangle( a, c, b );
    _boardMesh.AddTriangle( a, d, c );
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
        
    glEnable( GL_DEPTH_TEST );

    glEnable( GL_CULL_FACE );

    // stone shader, textures, VBs/IBs etc.
    {    
        _stoneProgram = [self loadShader:@"StoneShader"];

        void * vertexData = _stoneMesh.GetVertexBuffer();
        
        glGenBuffers( 1, &_stoneVertexBuffer );
        glBindBuffer( GL_ARRAY_BUFFER, _stoneVertexBuffer );
        glBufferData( GL_ARRAY_BUFFER, sizeof(Vertex) * _stoneMesh.GetNumVertices(), vertexData, GL_STATIC_DRAW );
        
        glEnableVertexAttribArray( GLKVertexAttribPosition );
        glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, 32, 0 );
        glEnableVertexAttribArray( GLKVertexAttribNormal );
        glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, 32, (void*)16 );
        
        glGenBuffers( 1, &_stoneIndexBuffer );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _stoneIndexBuffer );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, _stoneMesh.GetNumIndices()*sizeof(GLushort), _stoneMesh.GetIndexBuffer(), GL_STATIC_DRAW );

        _stoneUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _stoneProgram, "normalMatrix" );
        _stoneUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _stoneProgram, "modelViewProjectionMatrix" );
        _stoneUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _stoneProgram, "lightPosition" );
    }
    
    // board shader, textures, VBs/IBs etc.
    {
        _boardProgram = [self loadShader:@"BoardShader"];

        void * vertexData = _boardMesh.GetVertexBuffer();
        
        assert( vertexData );
        assert( _boardMesh.GetNumVertices() > 0 );
        assert( _boardMesh.GetNumIndices() > 0 );

        glGenBuffers( 1, &_boardVertexBuffer );

        glBindBuffer( GL_ARRAY_BUFFER, _boardVertexBuffer );

        glBufferData( GL_ARRAY_BUFFER, sizeof(TexturedVertex) * _boardMesh.GetNumVertices(), vertexData, GL_STATIC_DRAW );
        
        glGenBuffers( 1, &_boardIndexBuffer );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _boardIndexBuffer );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, _boardMesh.GetNumIndices()*sizeof(GLushort), _boardMesh.GetIndexBuffer(), GL_STATIC_DRAW );
        
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
    }
  
    // configure opengl view

    self.preferredFramesPerSecond = 60;

    self.view.multipleTouchEnabled = YES;
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
    
    glDeleteBuffers( 1, &_stoneIndexBuffer );
    glDeleteBuffers( 1, &_stoneVertexBuffer );
    
    if ( _stoneProgram )
    {
        glDeleteProgram( _stoneProgram );
        _stoneProgram = 0;
    }

    glDeleteBuffers( 1, &_boardIndexBuffer );
    glDeleteBuffers( 1, &_boardVertexBuffer );

    if ( _boardProgram )
    {
        glDeleteProgram( _boardProgram );
        _boardProgram = 0;
    }

    glDeleteTextures( 1, &_boardTexture );

    if ( _shadowProgram )
    {
        glDeleteProgram( _shadowProgram );
        _shadowProgram = 0;
    }
}

- (GLuint)loadTexture:(NSString *)filename
{    
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
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    GLfloat maxAnisotropy;
    glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy );

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

- (void)updateTouch:(float)dt
{
    if ( _selected )
    {
        _stone.rigidBody.angularMomentum *= SelectDamping;
        
        const float select_dt = max( 1.0f / 60.0f, _selectTimestamp - _selectPrevTimestamp );

        _stone.rigidBody.linearMomentum = _stone.rigidBody.mass *
            ( _selectIntersectionPoint - _selectPrevIntersectionPoint ) / select_dt;

        _stone.rigidBody.position = _selectIntersectionPoint + _selectOffset;

        _stone.rigidBody.Update();
    }
    else
    {
        if ( _holdStarted )
        {
            const float prevTime = _holdTime;

            _holdTime += dt;

            if ( prevTime < HoldDelay && _holdTime >= HoldDelay )
            {
//                NSLog( @"hold" );
            }

            if ( _holdTime >= HoldDelay )
                _stone.rigidBody.angularMomentum *= HoldDamping;
        }

        if ( _swipeStarted )
        {
            _swipeTime += dt;

            if ( _swipeTime > MaxSwipeTime )
                _swipeStarted = false;
        }
    }
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
//    NSLog( @"touches began" );
    
    UITouch * touch = [touches anyObject];

    if ( !_selected )
    {
        CGPoint selectPoint = [touch locationInView:self.view];

        // IMPORTANT: convert points to pixels!
        const float contentScaleFactor = [self.view contentScaleFactor];
        vec3f point( selectPoint.x, selectPoint.y, 0 );
        point *= contentScaleFactor; 

        vec3f rayStart, rayDirection;
        
        mat4f inverseClipMatrix;
        inverseClipMatrix.load( _inverseClipMatrix.m );
        
        GetPickRay( inverseClipMatrix, point.x(), point.y(), rayStart, rayDirection );
        
        RigidBodyTransform transform( _stone.rigidBody.position, _stone.rigidBody.orientation );

        vec3f intersectionPoint, intersectionNormal;

        if ( IntersectRayStone( _stone.biconvex, transform, rayStart, rayDirection, intersectionPoint, intersectionNormal ) > 0 )
        {
//            NSLog( @"select" );
            
            [self incrementCounter:COUNTER_SelectedStone];

            _selected = true;
            _selectTouch = touch;
            _selectPoint = selectPoint;
            _selectDepth = intersectionPoint.z();
            _selectOffset = _stone.rigidBody.position - intersectionPoint;
            _selectIntersectionPoint = intersectionPoint;
            _selectTimestamp = [touch timestamp];
            _selectPrevIntersectionPoint = _selectIntersectionPoint;
            _selectPrevTimestamp = _selectTimestamp;

            _stone.rigidBody.ApplyImpulseAtWorldPoint( intersectionPoint, vec3f(0,0,-TouchImpulse) );
        }
    }

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
    if ( _selected )
    {
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
                //NSLog( @"hold moved" );
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

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
//    NSLog( @"touches ended" );
    
    if ( !_selected )
    {
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

            if ( _zoomed )
                [self incrementCounter:COUNTER_ZoomedOut];
            else
                [self incrementCounter:COUNTER_ZoomedIn];
            
            _zoomed = !_zoomed;

            [NSObject cancelPreviousPerformRequestsWithTarget:self];
        }
    }
    else
    {
        if ( [touches containsObject:_selectTouch] )
        {
            UITouch * touch = _selectTouch;
            
            CGPoint selectPoint = [touch locationInView:self.view];
            
            const float dx = selectPoint.x - _selectPoint.x;
            const float dy = selectPoint.y - _selectPoint.y;

            const float distance = sqrt( dx*dx + dy*dy );

            if ( distance > 10 )
            {
                if ( length( _stone.rigidBody.linearVelocity ) > 20.0f )
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

void gluUnProject( GLfloat winx, GLfloat winy, GLfloat winz,
                   const mat4f inverseClipMatrix,
                   const GLint viewport[4],
                   GLfloat *objx, GLfloat *objy, GLfloat *objz )
{
    vec4f in( ( ( winx - viewport[0] ) / viewport[2] ) * 2 - 1,
              ( ( winy - viewport[1] ) / viewport[3] ) * 2 - 1,
              ( winz ) * 2 - 1,
              1.0f );

    vec4f out = inverseClipMatrix * in;
    
    *objx = out.x() / out.w();
    *objy = out.y() / out.w();
    *objz = out.z() / out.w();
}

void GetPickRay( const mat4f & inverseClipMatrix, float screen_x, float screen_y, vec3f & rayStart, vec3f & rayDirection )
{
    GLint viewport[4];
    glGetIntegerv( GL_VIEWPORT, viewport );

    const float displayHeight = viewport[3];

    float x = screen_x;
    float y = displayHeight - screen_y;

    GLfloat x1,y1,z1;
    GLfloat x2,y2,z2;

    gluUnProject( x, y, 0, inverseClipMatrix, viewport, &x1, &y1, &z1 );
    gluUnProject( x, y, 1, inverseClipMatrix, viewport, &x2, &y2, &z2 );

    vec3f ray1( x1,y1,z1 );
    vec3f ray2( x2,y2,z2 );

    rayStart = ray1;
    rayDirection = normalize( ray2 - ray1 );
}

- (void)handleSwipe:(vec3f)delta atPoint:(vec3f)point
{
//    NSLog( @"swipe" );
    const vec3f up = -normalize( _smoothedAcceleration );
    _stone.rigidBody.angularMomentum += SwipeMomentum * up;
    _stone.rigidBody.Update();

    [self incrementCounter:COUNTER_Swiped];

    _swipedThisFrame = true;
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    _holdStarted = false;
    _swipeStarted = false;
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
    _rawAcceleration = vec3f( acceleration.x, acceleration.y, acceleration.z );
    _smoothedAcceleration += ( _rawAcceleration - _smoothedAcceleration ) * AccelerometerTightness;
    _jerkAcceleration = _rawAcceleration - _smoothedAcceleration;
}

- (void)deviceOrientationDidChange:(NSNotification *)notification
{
    // ...
}

- (void)dropStone
{
    _stone.rigidBody.position = vec3f( 0, 0, 0 );
    _stone.rigidBody.orientation = quat4f(1,0,0,0);
    _stone.rigidBody.linearMomentum = vec3f(0,0,0);

    const float maxMoment = 1.0f;
    const float angularMoment = length( _stone.rigidBody.angularMomentum );
    if ( angularMoment > maxMoment )
        _stone.rigidBody.angularMomentum = normalize( _stone.rigidBody.angularMomentum ) * maxMoment;

    _stone.rigidBody.Update();
}

- (void)incrementCounter:(Counters)counterIndex
{
    if ( ++_counters[counterIndex] == 1 )
        [TestFlight passCheckpoint:[NSString stringWithUTF8String:CounterNames[counterIndex]]];
}

- (void)updatePhysics:(float)dt
{
    if ( !_hasRendered )
        return;

    Stone & stone = _stone;

    // calculate frustum planes for collision

    mat4f clipMatrix;
    clipMatrix.load( _clipMatrix.m );

    Frustum frustum;
    CalculateFrustumPlanes( clipMatrix, frustum );
    
    // apply jerk acceleration to stone

    const float jerk = length( _jerkAcceleration );
    if ( jerk > JerkThreshold )
        stone.rigidBody.linearMomentum += _jerkAcceleration * stone.rigidBody.mass * dt;

    // detect when the user has asked to launch the stone into the air
    
    const vec3f down = normalize( _smoothedAcceleration );
    const vec3f up = -down;

    if ( iPad() )
    {
        // for pads, let the launch go up in the air only!
        // other launches feel wrong and cause nausea -- treat the tablet like the board
        
        const float jerkUp = dot( _jerkAcceleration, up );
        
        if ( jerkUp > LaunchThreshold )
        {
            stone.rigidBody.linearMomentum += jerkUp * LaunchMomentum * up;
            stone.rigidBody.Update();

            [self incrementCounter:COUNTER_AppliedImpulse];
        }
    }
    else
    {
        // for the iphone let the player shake the stone around like a toy
        // the go stone is trapped inside the phone. this looks cool!
        
        if ( jerk > LaunchThreshold )
        {
            stone.rigidBody.linearMomentum += _jerkAcceleration * LaunchMomentum;
            stone.rigidBody.Update();
            
            [self incrementCounter:COUNTER_AppliedImpulse];
        }
    }
    
    // update stone physics

    _collisionPlanes = 0;
    
    bool collided = false;

    const int iterations = 10;

    const float iteration_dt = dt / iterations;

    for ( int i = 0; i < iterations; ++i )
    {        
        vec3f gravity = 9.8f * 10 * down;

        if ( !_selected )
            stone.rigidBody.linearMomentum += gravity * stone.rigidBody.mass * iteration_dt;

        stone.rigidBody.Update();

        if ( !_selected )
            stone.rigidBody.position += stone.rigidBody.linearVelocity * iteration_dt;

        const int rotation_substeps = 10;
        const float rotation_substep_dt = iteration_dt / rotation_substeps;
        for ( int j = 0; j < rotation_substeps; ++j )
        {
            quat4f spin = AngularVelocityToSpin( stone.rigidBody.orientation, stone.rigidBody.angularVelocity );
            stone.rigidBody.orientation += spin * rotation_substep_dt;
            stone.rigidBody.orientation = normalize( stone.rigidBody.orientation );
        }        
        
        // if just dropped let it first fall through the near plane
        // then it can start colliding with the frustum planes

        StaticContact contact;

        bool iteration_collided = false;
        
        const float e = 0.5f;
        const float u = 0.5f;
    
        // collision between stone and near plane

        vec4f nearPlane( 0, 0, -1, -_smoothZoom );//* ( ( !_zoomed && iPad() ) ? 0.7f : 1.0f ) );
        
        if ( StonePlaneCollision( stone.biconvex, nearPlane, stone.rigidBody, contact ) )
        {
            ApplyCollisionImpulseWithFriction( contact, e, u );
            stone.rigidBody.Update();
            iteration_collided = true;
            [self incrementCounter:COUNTER_HitNearPlane];
            _collisionPlanes |= COLLISION_NearPlane;
        }

        // collision between stone and left plane

        if ( StonePlaneCollision( stone.biconvex, frustum.left, stone.rigidBody, contact ) )
        {
            ApplyCollisionImpulseWithFriction( contact, e, u );
            stone.rigidBody.Update();
            iteration_collided = true;
            _collisionPlanes |= COLLISION_LeftPlane;
        }

        // collision between stone and right plane

        if ( StonePlaneCollision( stone.biconvex, frustum.right, stone.rigidBody, contact ) )
        {
            ApplyCollisionImpulseWithFriction( contact, e, u );
            stone.rigidBody.Update();
            iteration_collided = true;
            _collisionPlanes |= COLLISION_RightPlane;
        }

        // collision between stone and top plane

        if ( StonePlaneCollision( stone.biconvex, frustum.top, stone.rigidBody, contact ) )
        {
            ApplyCollisionImpulseWithFriction( contact, e, u );
            stone.rigidBody.Update();
            iteration_collided = true;
            _collisionPlanes |= COLLISION_TopPlane;
        }

        // collision between stone and bottom plane

        if ( StonePlaneCollision( stone.biconvex, frustum.bottom, stone.rigidBody, contact ) )
        {
            ApplyCollisionImpulseWithFriction( contact, e, u );
            stone.rigidBody.Update();
            iteration_collided = true;
            _collisionPlanes |= COLLISION_BottomPlane;
        }

        // collision between stone and board surface
        
        if ( StonePlaneCollision( stone.biconvex, vec4f(0,0,1,0), stone.rigidBody, contact ) )
        {
            ApplyCollisionImpulseWithFriction( contact, e, u );
            stone.rigidBody.Update();
            iteration_collided = true;
            _collisionPlanes |= COLLISION_GroundPlane;
        }
                
        // this is a *massive* hack to approximate rolling/spinning
        // friction and it is completely made up and not accurate at all!

        if ( iteration_collided )
        {
            float momentum = length( stone.rigidBody.angularMomentum );
            
            if ( momentum > 0 )
            {
                const float factor_a = 0.9925f;
                const float factor_b = 0.9995f;
                
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
            }
        }

        // apply damping

        const float linear_factor = 0.99999f;
        const float angular_factor = 0.99999f;

        stone.rigidBody.linearMomentum *= linear_factor;
        stone.rigidBody.angularMomentum *= angular_factor;
        
        stone.rigidBody.Update();
    }
}

- (void)updateDetection:(float)dt
{
    // update seconds since last swipe

    if ( _swipedThisFrame )
        _secondsSinceLastSwipe = 0;
    else
        _secondsSinceLastSwipe += dt;

    // update seconds since collision

    for ( int i = 0; i < COLLISION_NumValues; ++i )
        _secondsSinceCollision[i] += dt;

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

    const bool recentGroundCollision = _secondsSinceCollision[COLLISION_GroundPlane] < 0.1f;
    const bool recentLeftCollision = _secondsSinceCollision[COLLISION_LeftPlane] < 0.1f;
    const bool recentRightCollision = _secondsSinceCollision[COLLISION_RightPlane] < 0.1f;
    const bool recentTopCollision = _secondsSinceCollision[COLLISION_TopPlane] < 0.1f;
    const bool recentBottomCollision = _secondsSinceCollision[COLLISION_BottomPlane] < 0.1f;
    const bool recentNearCollision = _secondsSinceCollision[COLLISION_NearPlane] < 0.1f;

    // detect at rest on each collision plane

    const bool atRest = length( _stone.rigidBody.linearVelocity ) < 1.0f &&
                        length( _stone.rigidBody.angularVelocity ) < 0.5f;

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
                         length( _stone.rigidBody.linearVelocity ) < 10.0f &&
                         length( _stone.rigidBody.angularVelocity ) < 1.0f;

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

    const float SpinTime = 0.25f;

    const vec3f up = normalize( -_smoothedAcceleration );

    RigidBodyTransform rigidBodyTransform( _stone.rigidBody.position, _stone.rigidBody.orientation );
    
    const float upSpin = fabs( dot( _stone.rigidBody.angularVelocity, up ) );
    const float totalSpin = length( _stone.rigidBody.angularVelocity );

    const bool spinning = length( _stone.rigidBody.linearVelocity ) < 10.0 &&
                          dot( rigidBodyTransform.GetUp(), up ) < 0.25f &&
                          upSpin > 1.0f && fabs( upSpin / totalSpin ) > 0.7f &&
                          _secondsSinceLastSwipe < 1.0f;

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

    // update orientation detection

    const float OrientationTime = 1.0f;

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

    // clear values this frame

    _swipedThisFrame = false;
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
    
    RigidBodyTransform biconvexTransform( _stone.rigidBody.position, _stone.rigidBody.orientation );
    float opengl_transform[16];
    biconvexTransform.localToWorld.store( opengl_transform );

    GLKMatrix4 modelViewMatrix = GLKMatrix4MakeWithArray( opengl_transform );
    modelViewMatrix = GLKMatrix4Multiply( baseModelViewMatrix, modelViewMatrix );
    
    _stoneNormalMatrix = GLKMatrix3InvertAndTranspose( GLKMatrix4GetMatrix3(modelViewMatrix), NULL );
    _stoneModelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, modelViewMatrix );
    
    _boardNormalMatrix = GLKMatrix3InvertAndTranspose( GLKMatrix4GetMatrix3(baseModelViewMatrix), NULL );
    _boardModelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, baseModelViewMatrix );
    
    // todo: project onto ground plane
    _shadowNormalMatrix = GLKMatrix3InvertAndTranspose( GLKMatrix4GetMatrix3(modelViewMatrix), NULL );
    _shadowModelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, modelViewMatrix );

    _clipMatrix = GLKMatrix4Multiply( projectionMatrix, baseModelViewMatrix );

    bool invertible;
    _inverseClipMatrix = GLKMatrix4Invert( _clipMatrix, &invertible );
    assert( invertible );

    glClearColor( 0, 0, 0, 1 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    if ( _paused )
        return;

    // render board

    {
        glUseProgram( _boardProgram );
                
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

    // render stone shadow
    
    {
        glUseProgram( _shadowProgram );
        
        glBindBuffer( GL_ARRAY_BUFFER, _stoneVertexBuffer );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _stoneIndexBuffer );
        
        glEnableVertexAttribArray( GLKVertexAttribPosition );
        glEnableVertexAttribArray( GLKVertexAttribNormal );
        
        glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0 );
        glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)16 );
        
        glUniformMatrix4fv( _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _shadowModelViewProjectionMatrix.m );
        glUniformMatrix3fv( _shadowUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _shadowNormalMatrix.m );
        glUniform3fv( _shadowUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&_lightPosition );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        glDrawElements( GL_TRIANGLES, _stoneMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );

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

        glUniformMatrix4fv( _stoneUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _stoneModelViewProjectionMatrix.m );
        glUniformMatrix3fv( _stoneUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _stoneNormalMatrix.m );
        glUniform3fv( _stoneUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&_lightPosition );

        glDrawElements( GL_TRIANGLES, _stoneMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );

        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

        glDisableVertexAttribArray( GLKVertexAttribPosition );
        glDisableVertexAttribArray( GLKVertexAttribNormal );
    }
    
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
