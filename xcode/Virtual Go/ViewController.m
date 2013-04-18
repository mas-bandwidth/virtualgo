//
//  ViewController.m
//  Virtual Go
//
//  Created by Glenn Fiedler on 4/13/13.
//  Copyright (c) 2013 Glenn Fiedler. All rights reserved.
//

#import "ViewController.h"

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
    NUM_UNIFORMS
};

GLint uniforms[NUM_UNIFORMS];

enum
{
    ATTRIB_VERTEX,
    ATTRIB_NORMAL,
    NUM_ATTRIBUTES
};

@interface ViewController ()
{
    GLuint _program;
    
    GLKMatrix4 _modelViewProjectionMatrix;
    GLKMatrix3 _normalMatrix;
    GLKMatrix4 _clipMatrix;
    GLKMatrix4 _inverseClipMatrix;
    
    GLuint _vertexBuffer;
    GLuint _indexBuffer;
    
    Stone _stone;
    Mesh _mesh;
    
    bool _paused;
    bool _zoomed;
    bool _justDropped;
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

- (BOOL)loadShaders;
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

    GenerateBiconvexMesh( _mesh, _stone.biconvex, 4 );

    [self setupGL];

    _paused = true;
    _zoomed = false;
    _justDropped = false;
    _hasRendered = false;
    
    _smoothZoom = false;
    
    _rawAcceleration = vec3f(0,0,-1);
    _smoothedAcceleration = vec3f(0,0,-1);
    _jerkAcceleration = vec3f(0,0,0);

    _swipeStarted = false;
    _holdStarted = false;
    _selected = false;

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
    NSLog( @"did become active" );
    _paused = false;
    _hasRendered = false;
}

- (void)willResignActive:(NSNotification *)notification
{
    NSLog( @"will resign active" );
    _paused = true;
}

- (void)didEnterBackground:(NSNotification *)notification
{
    NSLog( @"did enter background" );
}

- (void)willEnterForeground:(NSNotification *)notification
{
    NSLog( @"will enter foreground" );
}

- (void)didReceiveMemoryWarning
{
    NSLog( @"did receive memory warning" );

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

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
    
    [self loadShaders];
    
    glEnable( GL_DEPTH_TEST );
    
    void * vertexData = _mesh.GetVertexBuffer();
    
    glGenBuffers( 1, &_vertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, _vertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(Vertex) * _mesh.GetNumVertices(), vertexData, GL_STATIC_DRAW );
    
    glEnableVertexAttribArray( GLKVertexAttribPosition );
    glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, 32, 0 );
    glEnableVertexAttribArray( GLKVertexAttribNormal);
    glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, 32, (void*)16 );
    
    glGenBuffers( 1, &_indexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _indexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, _mesh.GetNumIndices()*sizeof(GLuint), _mesh.GetIndexBuffer(), GL_STATIC_DRAW );
    
    self.preferredFramesPerSecond = 60;

    self.view.multipleTouchEnabled = YES;
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
    
    glDeleteBuffers( 1, &_indexBuffer );
    glDeleteBuffers( 1, &_vertexBuffer );
    
    if ( _program )
    {
        glDeleteProgram( _program );
        _program = 0;
    }
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
                NSLog( @"hold" );

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
    // todo: if a swipe is started and another different finger is place
    // on the ipad, cancel the swipe! --- swipe is a pure one finger motion
    
    NSLog( @"touches began" );
    
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
            NSLog( @"select" );

            _selected = true;
            _selectTouch = touch;
            _selectPoint = selectPoint;
            _selectDepth = intersectionPoint.z();
            _selectOffset = _stone.rigidBody.position - intersectionPoint;
            _selectIntersectionPoint = intersectionPoint;
            _selectTimestamp = [touch timestamp];
            _selectPrevIntersectionPoint = _selectIntersectionPoint;
            _selectPrevTimestamp = _selectTimestamp;
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

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSLog( @"touches ended" );
    
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
            NSLog( @"double tap" );

            _zoomed = !_zoomed;

            [NSObject cancelPreviousPerformRequestsWithTarget:self];
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
    NSLog( @"swipe" );
    const vec3f up = -normalize( _smoothedAcceleration );
    _stone.rigidBody.angularMomentum += SwipeMomentum * up;
    _stone.rigidBody.Update();
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
        NSLog( @"shake" );
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
    _stone.rigidBody.position = vec3f( 0, 0, _smoothZoom );
    _stone.rigidBody.orientation = quat4f(1,0,0,0);
    _stone.rigidBody.linearMomentum = vec3f(0,0,0);

    const float maxMoment = 1.0f;
    const float angularMoment = length( _stone.rigidBody.angularMomentum );
    if ( angularMoment > maxMoment )
        _stone.rigidBody.angularMomentum = normalize( _stone.rigidBody.angularMomentum ) * maxMoment;

    _stone.rigidBody.Update();

    _justDropped = true;
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
        }
    }
    
    // update stone physics
    
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
        
        if ( _justDropped )
        {
            const float r = stone.biconvex.GetBoundingSphereRadius();

            if ( stone.rigidBody.position.z() >= _smoothZoom - r )
            {
                stone.rigidBody.position = vec3f( 0, 0, min( stone.rigidBody.position.z(), _smoothZoom ) );
                if ( stone.rigidBody.linearMomentum.z() > 1 )
                    stone.rigidBody.linearMomentum = vec3f( stone.rigidBody.linearMomentum.x(), stone.rigidBody.linearMomentum.y(), 1 );
            }
            else
                _justDropped = false;
        }
        else
        {
            // collision between stone and near plane

            if ( StonePlaneCollision( stone.biconvex, frustum.front, stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, e, u );
                stone.rigidBody.Update();
                iteration_collided = true;
            }

            // collision between stone and left plane

            if ( StonePlaneCollision( stone.biconvex, frustum.left, stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, e, u );
                stone.rigidBody.Update();
                iteration_collided = true;
            }

            // collision between stone and right plane

            if ( StonePlaneCollision( stone.biconvex, frustum.right, stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, e, u );
                stone.rigidBody.Update();
                iteration_collided = true;
            }

            // collision between stone and top plane

            if ( StonePlaneCollision( stone.biconvex, frustum.top, stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, e, u );
                stone.rigidBody.Update();
                iteration_collided = true;
            }

            // collision between stone and bottom plane

            if ( StonePlaneCollision( stone.biconvex, frustum.bottom, stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, e, u );
                stone.rigidBody.Update();
                iteration_collided = true;
            }

            // collision between stone and board surface
            
            if ( StonePlaneCollision( stone.biconvex, vec4f(0,0,1,0), stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, e, u );
                stone.rigidBody.Update();
                iteration_collided = true;
            }
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

- (void)render
{
    float aspect = fabsf( self.view.bounds.size.width / self.view.bounds.size.height );
    
    const float targetZoom = iPad() ? ( _zoomed ? ZoomIn_iPad : ZoomOut_iPad ) : ( _zoomed ? ZoomIn_iPhone : ZoomOut_iPhone );
    
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
    
    _normalMatrix = GLKMatrix3InvertAndTranspose( GLKMatrix4GetMatrix3(modelViewMatrix), NULL );
    
    _modelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, modelViewMatrix );
    
    _clipMatrix = GLKMatrix4Multiply( projectionMatrix, baseModelViewMatrix );

    bool invertible;
    _inverseClipMatrix = GLKMatrix4Invert( _clipMatrix, &invertible );
    assert( invertible );

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
    
    [self render];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    if ( _justDropped )
        glClearColor( 1, 1, 1, 1 );
    else
        glClearColor( 0, 0, 0, 1 );
    
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    if ( _paused )
        return;
    
    const float r = _stone.biconvex.GetBoundingSphereRadius();
    
    if ( _justDropped && _stone.rigidBody.position.z() >= _smoothZoom - r )
        return;
    
    glUseProgram( _program );
    
    glUniformMatrix4fv( uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _modelViewProjectionMatrix.m );
    glUniformMatrix3fv( uniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _normalMatrix.m );
    
    glBindBuffer( GL_ARRAY_BUFFER, _vertexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _indexBuffer );
    
    glDrawElements( GL_TRIANGLES, _mesh.GetNumTriangles()*3, GL_UNSIGNED_INT, NULL );
}

#pragma mark -  OpenGL ES 2 shader compilation

- (BOOL)loadShaders
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;
    
    // Create shader program.
    _program = glCreateProgram();
    
    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"vsh"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname])
    {
        NSLog( @"Failed to compile vertex shader" );
        return NO;
    }
    
    // Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"fsh"];
    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname])
    {
        NSLog( @"Failed to compile fragment shader" );
        return NO;
    }
    
    // Attach vertex shader to program.
    glAttachShader( _program, vertShader );
    
    // Attach fragment shader to program.
    glAttachShader( _program, fragShader );
    
    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation( _program, GLKVertexAttribPosition, "position" );
    glBindAttribLocation( _program, GLKVertexAttribNormal, "normal" );
    
    // Link program.
    if (![self linkProgram:_program])
    {
        NSLog( @"Failed to link program: %d", _program );
        
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
        if ( _program )
        {
            glDeleteProgram( _program );
            _program = 0;
        }
        
        return NO;
    }
    
    // Get uniform locations.
    uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _program, "modelViewProjectionMatrix" );
    uniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _program, "normalMatrix" );
    
    // Release vertex and fragment shaders.
    if ( vertShader )
    {
        glDetachShader( _program, vertShader );
        glDeleteShader( vertShader );
    }
    if ( fragShader )
    {
        glDetachShader( _program, fragShader );
        glDeleteShader( fragShader );
    }
    
    return YES;
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
