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

    bool _swipeStarted;
    UITouch * _swipeTouch;
    float _swipeTime;
    CGPoint _swipeStartPoint;

    Mesh<Vertex> _stoneMesh;
    GLuint _stoneProgramWhite;
    GLuint _stoneProgramBlack;
    GLuint _stoneVertexBuffer;
    GLuint _stoneIndexBuffer;
    GLuint _stoneVAO;
    GLint _stoneUniformsBlack[NUM_UNIFORMS];
    GLint _stoneUniformsWhite[NUM_UNIFORMS];
    
    Mesh<TexturedVertex> _boardMesh;
    GLuint _boardProgram;
    GLuint _boardTexture;
    GLuint _boardVertexBuffer;
    GLuint _boardIndexBuffer;
    GLuint _boardVAO;
    GLint _boardUniforms[NUM_UNIFORMS];

    GLuint _shadowProgram;
    GLuint _shadowCompositingProgram;
    GLint _shadowUniforms[NUM_UNIFORMS];
    GLint _shadowCompositingUniforms[NUM_UNIFORMS];
    Mesh<Vertex> _stoneShadowMesh;
    Mesh<TexturedVertex> _boardQuadShadowMesh;
    GLuint _stoneShadowVertexBuffer;
    GLuint _stoneShadowIndexBuffer;
    GLuint _stoneShadowVAO;
    GLuint _boardQuadShadowVertexBuffer;
    GLuint _boardQuadShadowIndexBuffer;
    GLuint _boardQuadShadowVAO;
    GLuint _shadowBoardFramebuffer;
    GLuint _shadowBoardTexture;

    Mesh<TexturedVertex> _gridMesh;
    GLuint _gridProgram;
    GLuint _gridVertexBuffer;
    GLuint _gridIndexBuffer;
    GLuint _gridVAO;
    GLint _gridUniforms[NUM_UNIFORMS];
    GLuint _lineTexture;

    Mesh<TexturedVertex> _pointMesh;
    GLuint _pointProgram;
    GLuint _pointVertexBuffer;
    GLuint _pointIndexBuffer;
    GLuint _pointVAO;
    GLint _pointUniforms[NUM_UNIFORMS];
    GLuint _pointTexture;

    Mesh<TexturedVertex> _floorMesh;
    GLuint _floorProgram;
    GLuint _floorTexture;
    GLuint _floorVertexBuffer;
    GLuint _floorIndexBuffer;
    GLuint _floorVAO;
    GLint _floorUniforms[NUM_UNIFORMS];

    bool _paused;
    bool _rendered;
}

@property (strong, nonatomic) EAGLContext *context;

- (void)setupGL;
- (void)tearDownGL;

- (void)didBecomeActive:(NSNotification *)notification;
- (void)willResignActive:(NSNotification *)notification;
- (void)didEnterBackground:(NSNotification *)notification;
- (void)willEnterForeground:(NSNotification *)notification;
- (void)didReceiveMemoryWarning;

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.view.multipleTouchEnabled = ( MaxTouches > 1 ) ? YES : NO;

    UIAccelerometer * accel = [UIAccelerometer sharedAccelerometer];
    accel.updateInterval = 1 / AccelerometerFrequency;
    accel.delegate = self;

    telemetry.SetCounterNotifyFunc( HandleCounterNotify );

    const float aspectRatio = fabsf( self.view.bounds.size.width / self.view.bounds.size.height );

    game.Initialize( telemetry, accelerometer, 1.0f / aspectRatio );        // hack: for landscape!

    GenerateBiconvexMesh( _stoneMesh, game.GetBiconvex(), StoneTessellationLevel );
    GenerateBiconvexMesh( _stoneShadowMesh, game.GetShadowBiconvex(), StoneShadowTessellationLevel );
    GenerateFloorMesh( _floorMesh );
    GenerateBoardMesh( _boardMesh, game.GetBoard() );
    GenerateGridMesh( _gridMesh, game.GetBoard() );
    GenerateStarPointsMesh( _pointMesh, game.GetBoard() );

    {
        const float w = game.GetBoard().GetWidth() / 2;
        const float h = game.GetBoard().GetHeight() / 2;
        const float z = game.GetBoard().GetThickness() + 0.01f;
        GenerateQuadMesh( _boardQuadShadowMesh,
                          vec3f( -w, h, z ),
                          vec3f( +w, h, z ),
                          vec3f( +w, -h, z ),
                          vec3f( -w, -h, z ),
                          vec3f( 0, 0, 1 ) );
    }

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
    
    _swipeStarted = false;

    _paused = true;
    _rendered = false;

    [self setupNotifications];
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

        [opengl generateVBAndIBFromMesh:_stoneMesh vertexBuffer:_stoneVertexBuffer indexBuffer:_stoneIndexBuffer vertexArrayObject:_stoneVAO];

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

        [opengl generateVBAndIBFromTexturedMesh:_boardMesh vertexBuffer:_boardVertexBuffer indexBuffer:_boardIndexBuffer vertexArrayObject:_boardVAO];
        
        _boardTexture = [opengl loadTexture:@"wood.jpg"];

        _boardUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _boardProgram, "normalMatrix" );
        _boardUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _boardProgram, "modelViewProjectionMatrix" );
        _boardUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _boardProgram, "lightPosition" );
    }

    // shadow shader, uniforms, render to texture framebuffer etc.
    {
        _shadowProgram = [opengl loadShader:@"ShadowShader"];
        _shadowCompositingProgram = [opengl loadShader:@"ShadowCompositingShader"];

        [opengl generateVBAndIBFromMesh:_stoneShadowMesh vertexBuffer:_stoneShadowVertexBuffer indexBuffer:_stoneShadowIndexBuffer vertexArrayObject:_stoneShadowVAO];

        [opengl generateVBAndIBFromTexturedMesh:_boardQuadShadowMesh vertexBuffer:_boardQuadShadowVertexBuffer indexBuffer:_boardQuadShadowIndexBuffer vertexArrayObject:_boardQuadShadowVAO];

        _shadowUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _shadowProgram, "normalMatrix" );
        _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _shadowProgram, "modelViewProjectionMatrix" );
        _shadowUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _shadowProgram, "lightPosition" );
        _shadowUniforms[UNIFORM_ALPHA] = glGetUniformLocation( _shadowProgram, "alpha" );

        _shadowCompositingUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _shadowCompositingProgram, "normalMatrix" );
        _shadowCompositingUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _shadowCompositingProgram, "modelViewProjectionMatrix" );
        _shadowCompositingUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _shadowCompositingProgram, "lightPosition" );
        _shadowCompositingUniforms[UNIFORM_ALPHA] = glGetUniformLocation( _shadowCompositingProgram, "alpha" );

        glGenFramebuffers( 1, &_shadowBoardFramebuffer );

        glBindFramebuffer( GL_FRAMEBUFFER, _shadowBoardFramebuffer );

        glGenTextures( 1, &_shadowBoardTexture );
        glBindTexture( GL_TEXTURE_2D, _shadowBoardTexture );        
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, RenderToTextureSize, RenderToTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _shadowBoardTexture, 0 );

        glBindTexture( GL_TEXTURE_2D, 0 );

        glBindFramebuffer( GL_FRAMEBUFFER, _shadowBoardFramebuffer );

        if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
        {
            printf( "error: framebuffer is not complete!\n" );
            assert( false );
        }

        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    }
  
    // grid shader, textures, VBs/IBs etc.
    {
        _gridProgram = [opengl loadShader:@"GridShader"];

        [opengl generateVBAndIBFromTexturedMesh:_gridMesh vertexBuffer:_gridVertexBuffer indexBuffer:_gridIndexBuffer vertexArrayObject:_gridVAO];
        
        _gridUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _gridProgram, "normalMatrix" );
        _gridUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _gridProgram, "modelViewProjectionMatrix" );
        _gridUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _gridProgram, "lightPosition" );

        _lineTexture = [opengl loadTexture:@"line.jpg"];
    }

    // star points
    {
        _pointProgram = [opengl loadShader:@"PointShader"];

        [opengl generateVBAndIBFromTexturedMesh:_pointMesh vertexBuffer:_pointVertexBuffer indexBuffer:_pointIndexBuffer vertexArrayObject:_pointVAO];

        _pointUniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation( _pointProgram, "normalMatrix" );
        _pointUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation( _pointProgram, "modelViewProjectionMatrix" );
        _pointUniforms[UNIFORM_LIGHT_POSITION] = glGetUniformLocation( _pointProgram, "lightPosition" );

        _pointTexture = [opengl loadTexture:@"point.jpg"];
    }

    // floor shader, textures, VBs/IBs etc.
    {
        _floorProgram = [opengl loadShader:@"FloorShader"];

        [opengl generateVBAndIBFromTexturedMesh:_floorMesh vertexBuffer:_floorVertexBuffer indexBuffer:_floorIndexBuffer vertexArrayObject:_floorVAO];
        
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
    [opengl destroyBuffer:_stoneVAO];
    [opengl destroyProgram:_stoneProgramBlack];
    [opengl destroyProgram:_stoneProgramWhite];

    [opengl destroyBuffer:_boardIndexBuffer];
    [opengl destroyBuffer:_boardVertexBuffer];
    [opengl destroyBuffer:_boardVAO];
    [opengl destroyProgram:_boardProgram];
    [opengl destroyTexture:_boardTexture];

    [opengl destroyProgram:_shadowProgram];
    [opengl destroyProgram:_shadowCompositingProgram];
    [opengl destroyBuffer:_stoneShadowIndexBuffer];
    [opengl destroyBuffer:_stoneShadowVertexBuffer];
    [opengl destroyBuffer:_stoneShadowVAO];
    [opengl destroyBuffer:_boardQuadShadowVertexBuffer];
    [opengl destroyBuffer:_boardQuadShadowIndexBuffer];
    [opengl destroyBuffer:_boardQuadShadowVAO];
    [opengl destroyTexture:_shadowBoardTexture];
    [opengl destroyFramebuffer:_shadowBoardFramebuffer];
    
    [opengl destroyProgram:_gridProgram];
    [opengl destroyBuffer:_gridIndexBuffer];
    [opengl destroyBuffer:_gridVertexBuffer];
    [opengl destroyBuffer:_gridVAO];
    [opengl destroyTexture:_lineTexture];
    
    [opengl destroyProgram:_pointProgram];
    [opengl destroyBuffer:_floorIndexBuffer];
    [opengl destroyBuffer:_floorVertexBuffer];
    [opengl destroyBuffer:_floorVAO];
    [opengl destroyTexture:_pointTexture];

    [opengl destroyBuffer:_floorIndexBuffer];
    [opengl destroyBuffer:_floorVertexBuffer];
    [opengl destroyBuffer:_floorVAO];
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

- (void)didBecomeActive:(NSNotification *)notification
{
//    NSLog( @"did become active" );
    _paused = false;
    _rendered = false;
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

- (vec3f)pointToPixels:(CGPoint)point
{
    const float contentScaleFactor = [self.view contentScaleFactor];
    vec3f pixels( point.x, point.y, 0 );
    pixels *= contentScaleFactor;
    return pixels;
}

- (void)convertTouches:(NSSet*)nativeTouches toArray:(Touch*)touches andCount:(int&)numTouches
{
    NSArray * array = [nativeTouches allObjects];
    numTouches = [array count];
    assert( numTouches <= MaxTouches );
    for ( int i = 0; i < numTouches; ++i )
    {
        UITouch * nativeTouch = [array objectAtIndex:i];
        touches[i].handle = (__bridge TouchHandle)nativeTouch;
        touches[i].point = [self pointToPixels:[nativeTouch locationInView:self.view]];
        touches[i].timestamp = [nativeTouch timestamp];
    }
}

- (void)touchesBegan:(NSSet *)nativeTouches withEvent:(UIEvent *)event
{
    [super touchesBegan:nativeTouches withEvent:event];

    int numTouches = 0;
    Touch touches[MaxTouches];
    [self convertTouches:nativeTouches toArray:touches andCount:numTouches];

    game.OnTouchesBegan( touches, numTouches );

    if ( !_swipeStarted )
    {
        UITouch * touch = [nativeTouches anyObject];
        _swipeTouch = touch;
        _swipeTime = 0;
        _swipeStarted = true;
        _swipeStartPoint = [touch locationInView:self.view];
    }
}

- (void)touchesMoved:(NSSet *)nativeTouches withEvent:(UIEvent *)event
{
    [super touchesMoved:nativeTouches withEvent:event];

    int numTouches = 0;
    Touch touches[MaxTouches];
    [self convertTouches:nativeTouches toArray:touches andCount:numTouches];

    game.OnTouchesMoved( touches, numTouches );
}
    
- (void)touchesEnded:(NSSet *)nativeTouches withEvent:(UIEvent *)event
{
    [super touchesEnded:nativeTouches withEvent:event];

    int numTouches = 0;
    Touch touches[MaxTouches];
    [self convertTouches:nativeTouches toArray:touches andCount:numTouches];

    if ( _swipeStarted && [nativeTouches containsObject:_swipeTouch] )
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

            game.OnSwipe( swipePoint, swipeDelta );
        }

        _swipeStarted = false;
    }

    game.OnTouchesEnded( touches, numTouches );

    UITouch * touch = [nativeTouches anyObject];

    if ( touch.tapCount >= 2 )
    {
        CGPoint touchPoint = [touch locationInView:self.view];

        game.OnDoubleTap( [self pointToPixels:touchPoint] );
        
        [NSObject cancelPreviousPerformRequestsWithTarget:self];
    }
}

- (void)touchesCancelled:(NSSet *)nativeTouches withEvent:(UIEvent *)event
{
    [super touchesCancelled:nativeTouches withEvent:event];

    int numTouches = 0;
    Touch touches[MaxTouches];
    [self convertTouches:nativeTouches toArray:touches andCount:numTouches];

    if ( _swipeStarted && [nativeTouches containsObject:_swipeTouch] )
        _swipeStarted = false;

    game.OnTouchesCancelled( touches, numTouches );
}

- (void)accelerometer:(UIAccelerometer *)uiAccelerometer didAccelerate:(UIAcceleration *)acceleration
{
    accelerometer.Update( vec3f( acceleration.x, acceleration.y, acceleration.z ) );
}

- (void)render
{
#if SHADOWS
    
    GLint defaultFBO;
    glGetIntegerv( GL_FRAMEBUFFER_BINDING_OES, &defaultFBO );

    const vec3f & lightPosition = game.GetLightPosition();

    // setup render to texture
    
    glBindTexture( GL_TEXTURE_2D, 0 );
    
    glBindFramebuffer( GL_FRAMEBUFFER, _shadowBoardFramebuffer );
    
    glViewport( 0, 0, RenderToTextureSize, RenderToTextureSize );
    
    // render to texture for shadows on board

    glClearColor( 0, 0, 0, 0 );
    
    glClear( GL_COLOR_BUFFER_BIT );

    glUseProgram( _shadowProgram );
    
    glBindVertexArrayOES( _stoneShadowVAO );

    glDisable( GL_DEPTH_TEST );
    
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    
    const std::vector<StoneInstance> & stones = game.GetStones();

    const float w = game.GetBoard().GetWidth() / 2;
    const float h = game.GetBoard().GetHeight() / 2;

    mat4f camera = mat4f::lookAt( vec3f(0,0,10), vec3f(0,0,0), vec3f(0,1,0) );
    mat4f orthoProjection = mat4f::ortho( -w, w, +h, -h, -100, +100 );

    for ( int i = 0; i < stones.size(); ++i )
    {
        const StoneInstance & stone = stones[i];
        
        if ( stone.rigidBody.position.z() < game.GetBoard().GetThickness() )
            continue;

        mat4f shadowMatrix;
        MakeShadowMatrix( vec4f(0,0,1,game.GetBoard().GetThickness()), vec4f( lightPosition.x(), lightPosition.y(), lightPosition.z(), 1 ), shadowMatrix );

        mat4f modelView = camera * shadowMatrix * stone.visualTransform;
        mat4f modelViewProjectionMatrix = orthoProjection * modelView;

        glUniformMatrix4fv( _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&modelViewProjectionMatrix );
        
        float shadowAlpha = GetShadowAlpha( stone.rigidBody.position );
        if ( shadowAlpha == 0.0f )
            continue;
        glUniform1fv( _shadowUniforms[UNIFORM_ALPHA], 1, (float*)&shadowAlpha );
        
        glDrawElements( GL_TRIANGLES, _stoneShadowMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
    }

    glDisable( GL_BLEND );

    glEnable( GL_DEPTH_TEST );

    // render to the default framebuffer
    
    glBindFramebuffer( GL_FRAMEBUFFER, defaultFBO );

    const float viewport_s = [self.view contentScaleFactor];
    const float viewport_w = viewport_s * self.view.bounds.size.width;
    const float viewport_h = viewport_s * self.view.bounds.size.height;
    
    glViewport( 0, 0, viewport_w, viewport_h );
    
#endif

    // ***** NORMAL RENDERING BELOW THIS LINE *****

    glClearColor( 0, 0, 0, 1 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
#if !STONE_DEMO

    // render floor

    {
        glUseProgram( _floorProgram );
                
        glBindTexture( GL_TEXTURE_2D, _floorTexture );

        glBindVertexArrayOES( _floorVAO );

        glUniformMatrix4fv( _floorUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&game.GetClipMatrix() );
        glUniformMatrix3fv( _floorUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&game.GetNormalMatrix() );
        glUniform3fv( _floorUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.GetLightPosition() );

        glDrawElements( GL_TRIANGLES, _floorMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
    }

#endif

    // render board

    {
        glUseProgram( _boardProgram );
        
        glBindTexture( GL_TEXTURE_2D, _boardTexture );

        glBindVertexArrayOES( _boardVAO );

        glUniformMatrix4fv( _boardUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&game.GetClipMatrix() );
        glUniformMatrix3fv( _boardUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&game.GetNormalMatrix() );
        glUniform3fv( _boardUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.GetLightPosition() );

        glDrawElements( GL_TRIANGLES, _boardMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
    }    

    // render white stones

    {
        glUseProgram( _stoneProgramWhite );
                
        glBindTexture( GL_TEXTURE_2D, 0 );

        glBindVertexArrayOES( _stoneVAO );

        glUniform3fv( _stoneUniformsWhite[UNIFORM_LIGHT_POSITION], 1, (float*)&lightPosition );

        const std::vector<StoneInstance> & stones = game.GetStones();

        for ( int i = 0; i < stones.size(); ++i )
        {
            const StoneInstance & stone = stones[i];
            
            if ( !stone.white )
                continue;
            
            mat4f modelViewMatrix = game.GetCameraMatrix() * stone.visualTransform;
            
            mat3f stoneNormalMatrix;
            stoneNormalMatrix.load( modelViewMatrix );

            mat4f stoneModelViewProjectionMatrix = game.GetProjectionMatrix() * modelViewMatrix;

            glUniformMatrix4fv( _stoneUniformsWhite[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&stoneModelViewProjectionMatrix );
            glUniformMatrix3fv( _stoneUniformsWhite[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&stoneNormalMatrix );

            glDrawElements( GL_TRIANGLES, _stoneMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        }
    }

#if !STONE_DEMO

    // render black stones
    
    {
        glUseProgram( _stoneProgramBlack );
        
        glBindVertexArrayOES( _stoneVAO );
        
        glUniform3fv( _stoneUniformsBlack[UNIFORM_LIGHT_POSITION], 1, (float*)&lightPosition );

        const std::vector<StoneInstance> & stones = game.GetStones();

        for ( int i = 0; i < stones.size(); ++i )
        {
            const StoneInstance & stone = stones[i];
            
            if ( stone.white )
                continue;
            
            mat4f modelViewMatrix = game.GetCameraMatrix() * stone.visualTransform;
            
            mat3f stoneNormalMatrix;
            stoneNormalMatrix.load( modelViewMatrix );
            
            mat4f stoneModelViewProjectionMatrix = game.GetProjectionMatrix() * modelViewMatrix;
            
            glUniformMatrix4fv( _stoneUniformsBlack[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&stoneModelViewProjectionMatrix );
            glUniformMatrix3fv( _stoneUniformsBlack[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&stoneNormalMatrix );
            
            glDrawElements( GL_TRIANGLES, _stoneMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        }
    }

#endif
    
    // *** IMPORTANT: RENDER ALL ALPHA BLENDED OBJECTS BELOW THIS LINE ***
    
    // render grid
    
    {
        glUseProgram( _gridProgram );
        
        glBindTexture( GL_TEXTURE_2D, _lineTexture );
        
        glBindVertexArrayOES( _gridVAO );
        
        glUniformMatrix4fv( _gridUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&game.GetClipMatrix() );
        glUniformMatrix3fv( _gridUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&game.GetNormalMatrix() );
        glUniform3fv( _gridUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.GetLightPosition() );
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_ZERO, GL_SRC_COLOR );
        
        glDrawElements( GL_TRIANGLES, _gridMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        
        glDisable( GL_BLEND );
    }
    
    // render star points
    
    {
        glUseProgram( _pointProgram );
        
        glBindTexture( GL_TEXTURE_2D, _pointTexture );
        
        glBindVertexArrayOES( _pointVAO );
        
        glUniformMatrix4fv( _pointUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&game.GetClipMatrix() );
        glUniformMatrix3fv( _pointUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&game.GetNormalMatrix() );
        glUniform3fv( _pointUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.GetLightPosition() );
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_ZERO, GL_SRC_COLOR );
        
        glDrawElements( GL_TRIANGLES, _pointMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        
        glDisable( GL_BLEND );
    }
    
#if SHADOWS
    
    // now render the shadow quad on the board
    
    {
        glUseProgram( _shadowCompositingProgram );
        
        glBindTexture( GL_TEXTURE_2D, _shadowBoardTexture );
        
        glBindVertexArrayOES( _boardQuadShadowVAO );
        
        glUniformMatrix4fv( _shadowCompositingUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&game.GetClipMatrix() );
        glUniformMatrix3fv( _shadowCompositingUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&game.GetNormalMatrix() );
        glUniform3fv( _shadowCompositingUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.GetLightPosition() );
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );
        
        glDrawElements( GL_TRIANGLES, _boardQuadShadowMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        
        glDisable( GL_BLEND );
    }
    
#endif

    // we no longer need the depth buffer. discard it
    {
        const GLenum discards[]  = { GL_DEPTH_ATTACHMENT };
        glDiscardFramebufferEXT( GL_FRAMEBUFFER, 1, discards );
    }
}

- (void)update
{
    if ( _paused )//|| !_rendered )
        return;

    float dt = 1.0f / 60.0f;

    game.Update( dt );

    if ( _swipeStarted )
    {
        _swipeTime += dt;

        if ( _swipeTime > MaxSwipeTime )
            _swipeStarted = false;
    }

    telemetry.Update( dt, game.GetBoard(), game.GetStones(), game.IsLocked(), accelerometer.GetUp() );
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    if ( _paused )
    {
        _rendered = false;
        return;
    }

    _rendered = true;

    // IMPORTANT: otherwise we may not have correct matrices
    game.UpdateCamera();

    [self render];
}

@end
