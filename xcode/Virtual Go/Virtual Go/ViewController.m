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
    float _rotation;
    
    GLuint _vertexBuffer;
    GLuint _indexBuffer;
    
    Stone _stone;
    Mesh _mesh;
    
    bool _paused;
    bool _pendingUnpause;
    
    bool _zoomed;
    float _smoothZoom;
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

const float ZoomIn = 5;
const float ZoomOut = 12;
const float ZoomInTightness = 0.25f;
const float ZoomOutTightness = 0.15f;

- (void)viewDidLoad
{
    [super viewDidLoad];

    [self setupNotifications];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context)
    {
        NSLog( @"Failed to create ES context" );
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    
    _stone.Initialize( STONE_SIZE_34 );
    
    GenerateBiconvexMesh( _mesh, _stone.biconvex );
    
    _paused = true;
    _pendingUnpause = false;
    
    _zoomed = false;
    _smoothZoom = ZoomOut;

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
    _pendingUnpause = true;
}

- (void)willResignActive:(NSNotification *)notification
{
    NSLog( @"will resign active" );
    _paused = true;
    _pendingUnpause = false;
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
    // ...
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
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
}

- (void)motionBegan:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
    NSLog( @"motion began" );
}

- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
    NSLog( @"motion ended" );
}

- (void)motionCancelled:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
    NSLog( @"motion cancelled" );
}

/*
 const int AccelerometerFrequency = 60;
 
 - (void)configureAccelerometer
 {
 UIAccelerometer * accelerometer = [UIAccelerometer sharedAccelerometer];
 accelerometer.updateInterval = 1 / AccelerometerFrequency;
 accelerometer.delegate = self;
 }
 */

- (void)deviceOrientationDidChange:(NSNotification *)notification
{
    NSLog( @"device orientation did change" );
}

- (void)update
{
    float aspect = fabsf( self.view.bounds.size.width / self.view.bounds.size.height );
    
    const float targetZoom = _zoomed ? ZoomIn : ZoomOut;
    
    _smoothZoom += ( targetZoom - _smoothZoom ) * ( _zoomed ? ZoomInTightness : ZoomOutTightness );
    
    GLKMatrix4 projectionMatrix = GLKMatrix4MakePerspective( GLKMathDegreesToRadians(40.0f), aspect, 0.1f, 100.0f );
    
    GLKMatrix4 baseModelViewMatrix = GLKMatrix4MakeLookAt( 0, -_smoothZoom, 0,
                                                           0, 0, 0,
                                                           0, 0, 1 );
    
    GLKMatrix4 modelViewMatrix = GLKMatrix4Identity;
    modelViewMatrix = GLKMatrix4Rotate( modelViewMatrix, _rotation, 1, 1, 1 );
    modelViewMatrix = GLKMatrix4Multiply( baseModelViewMatrix, modelViewMatrix );
    
    _normalMatrix = GLKMatrix3InvertAndTranspose( GLKMatrix4GetMatrix3(modelViewMatrix), NULL );
    
    _modelViewProjectionMatrix = GLKMatrix4Multiply( projectionMatrix, modelViewMatrix );
    
    float dt = self.timeSinceLastUpdate;
    if ( dt > 1 / 10.0f )
        dt = 1 / 10.0f;
    
    if ( _paused )
        dt = 0.0f;
    
    _rotation += dt * 0.5f;
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glClearColor( 0.2f, 0.2f, 0.2f, 1.0f );
    
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    glUseProgram( _program );
    
    glUniformMatrix4fv( uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _modelViewProjectionMatrix.m );
    glUniformMatrix3fv( uniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _normalMatrix.m );
    
    glBindBuffer( GL_ARRAY_BUFFER, _vertexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _indexBuffer );

    glDrawElements( GL_TRIANGLES, _mesh.GetNumTriangles()*3, GL_UNSIGNED_INT, NULL );
    
    // IMPORTANT: attempt avoid the pop when restoring the app
    // this does not seem to work. i need to understand what is going on
    if ( _pendingUnpause )
    {
        _pendingUnpause = false;
        _paused = false;
        NSLog( @"pending unpause -> unpause" );
    }
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
