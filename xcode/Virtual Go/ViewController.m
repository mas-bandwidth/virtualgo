//
//  ViewController.m
//  Virtual Go
//
//  Created by Glenn Fiedler on 4/13/13.
//  Copyright (c) 2013 Glenn Fiedler. All rights reserved.
//

#import "ViewController.h"

#include "Config.h"
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
    
    GLuint _vertexBuffer;
    GLuint _indexBuffer;
    
    Stone _stone;
    Mesh _mesh;
    
    bool _paused;
    
    bool _zoomed;
    float _smoothZoom;
    
    vec3f _rawAcceleration;                 // raw data from the accelometer
    vec3f _smoothedAcceleration;            // smoothed acceleration = gravity
    vec3f _jerkAcceleration;                // jerk acceleration = short motions, above a threshold = bump the board
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

const float AccelerometerFrequency = 60;
const float AccelerometerTightness = 0.1f;
const float JerkThreshold = 0.25f;

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
    
    _stone.Initialize( STONE_SIZE_34 );
    
    _stone.rigidBody.position = vec3f( 0, 0, 20 );//_stone.biconvex.GetHeight()/2 );
    _stone.rigidBody.orientation = quat4f(1,0,0,0);
    _stone.rigidBody.linearVelocity = vec3f(0,0,0);
    _stone.rigidBody.angularVelocity = vec3f(0,0,0);
    _stone.rigidBody.Update();
    
    GenerateBiconvexMesh( _mesh, _stone.biconvex );
    
    _paused = true;
    _zoomed = iPad();
    
    _smoothZoom = iPad() ? ZoomOut_iPad : ZoomOut_iPhone;
    
    _rawAcceleration = vec3f(0,0,-1);
    _smoothedAcceleration = vec3f(0,0,-1);
    _jerkAcceleration = vec3f(0,0,0);

    [self setupGL];
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

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent *)event
{
    NSLog( @"touches began" );
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    UITouch * touch = [touches anyObject];
    
    if ( touch.tapCount == 1 )
    {
        
        NSDictionary * touchLoc = [NSDictionary dictionaryWithObject:
                                   [NSValue valueWithCGPoint:[touch locationInView:self.view]] forKey:@"location"];
        
        [self performSelector:@selector(handleSingleTap:) withObject:touchLoc afterDelay:0.2];
        
    }
    else if ( touch.tapCount >= 2 )
    {
        NSLog( @"double tap" );

        _zoomed = !_zoomed;

        [NSObject cancelPreviousPerformRequestsWithTarget:self];
    }
}

- (void)handleSingleTap:(NSDictionary *)touches
{
    NSLog( @"single tap" );
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
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

- (void)updatePhysics:(float)dt
{
    if ( dt == 0 )
        return;
    
    bool collided = false;
    
    Stone & stone = _stone;
    
    // update stone physics

    const int iterations = 20;

    const float iteration_dt = dt / iterations;

    for ( int i = 0; i < iterations; ++i )
    {
        const float gravity = 9.8f * 10;    // cms/sec^2

        stone.rigidBody.linearMomentum += vec3f(0,0,-gravity) * stone.rigidBody.mass * iteration_dt;

        stone.rigidBody.Update();

        stone.rigidBody.position += stone.rigidBody.linearVelocity * iteration_dt;

        const int rotation_substeps = 10;
        const float rotation_substep_dt = iteration_dt / rotation_substeps;
        for ( int j = 0; j < rotation_substeps; ++j )
        {
            quat4f spin = AngularVelocityToSpin( stone.rigidBody.orientation, stone.rigidBody.angularVelocity );
            stone.rigidBody.orientation += spin * rotation_substep_dt;
            stone.rigidBody.orientation = normalize( stone.rigidBody.orientation );
        }

        // collision between stone and board

        /*
        collided = false;

        const float board_e = 0.8f;
        const float board_u = 0.1f;

        StaticContact boardContact;
        if ( StoneFloorCollision( stone.biconvex, stone.rigidBody, boardContact ) )
        {
            ApplyCollisionImpulseWithFriction( boardContact, board_e, board_u );
            stone.rigidBody.Update();
            collided = true;
        }
        */

        // this is a *massive* hack to approximate rolling/spinning
        // friction and it is completely made up and not accurate at all!

        if ( collided )
        {
            float momentum = length( stone.rigidBody.angularMomentum );
            const float factor_a = DecayFactor( 0.9915f, dt );
            const float factor_b = DecayFactor( 0.9995f, dt );
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
        }

        // apply damping

        const float linear_factor = DecayFactor( 0.99999f, dt );
        const float angular_factor = DecayFactor( 0.9999f, dt );

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
    glMultMatrixf( opengl_transform );

    GLKMatrix4 modelViewMatrix = GLKMatrix4MakeWithArray( opengl_transform );
    
    modelViewMatrix = GLKMatrix4Multiply( modelViewMatrix, baseModelViewMatrix );
    
    _normalMatrix = GLKMatrix3InvertAndTranspose( GLKMatrix4GetMatrix3(modelViewMatrix), NULL );
    
    _modelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, modelViewMatrix );
}

- (void)update
{
    if ( length( _jerkAcceleration ) > JerkThreshold )
        NSLog( @"jerk acceleration: %f,%f,%f", _jerkAcceleration.x(), _jerkAcceleration.y(), _jerkAcceleration.z() );
    
    float dt = self.timeSinceLastUpdate;
    if ( dt > 1 / 10.0f )
        dt = 1 / 10.0f;
    
    if ( _paused )
        dt = 0.0f;
    
    [self updatePhysics:dt];
    
    [self render];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );
    
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    if ( _paused )
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
