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
}

@property (strong, nonatomic) EAGLContext *context;

- (void)setupGL;
- (void)tearDownGL;

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event;

- (void)deviceOrientationDidChange:(NSNotification *)notification;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.view.multipleTouchEnabled = YES;

    telemetry.SetCounterNotifyFunc( HandleCounterNotify );

    const float aspectRatio = fabsf( self.view.bounds.size.width / self.view.bounds.size.height );

    game.Initialize( telemetry, accelerometer, 1.0f / aspectRatio );        // hack: for landscape!

    GenerateBiconvexMesh( _stoneMesh, game.GetBiconvex(), 3 );
    GenerateFloorMesh( _floorMesh );
    GenerateBoardMesh( _boardMesh, game.GetBoard() );
    GenerateGridMesh( _gridMesh, game.GetBoard() );
    GenerateStarPointsMesh( _pointMesh, game.GetBoard() );

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
    
    [ [NSNotificationCenter defaultCenter] addObserver : self
                                              selector : @selector(deviceOrientationDidChange:)
                                                  name : UIDeviceOrientationDidChangeNotification object:nil ];
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

- (void)deviceOrientationDidChange:(NSNotification *)notification
{
    /*
    const float aspectRatio = fabsf( self.view.bounds.size.width / self.view.bounds.size.height );

    game.SetAspectRatio( 1.0f / aspectRatio );
     */
}

- (vec3f) pointToPixels:(CGPoint)point
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
    int numTouches = 0;
    Touch touches[MaxTouches];
    [self convertTouches:nativeTouches toArray:touches andCount:numTouches];

    game.OnTouchesBegan( touches, numTouches );
}

- (void)touchesMoved:(NSSet *)nativeTouches withEvent:(UIEvent *)event
{
    int numTouches = 0;
    Touch touches[MaxTouches];
    [self convertTouches:nativeTouches toArray:touches andCount:numTouches];

    game.OnTouchesMoved( touches, numTouches );
}

- (void)touchesEnded:(NSSet *)nativeTouches withEvent:(UIEvent *)event
{
    int numTouches = 0;
    Touch touches[MaxTouches];
    [self convertTouches:nativeTouches toArray:touches andCount:numTouches];

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
    int numTouches = 0;
    Touch touches[MaxTouches];
    [self convertTouches:nativeTouches toArray:touches andCount:numTouches];

    game.OnTouchesCancelled( touches, numTouches );
}

- (void)accelerometer:(UIAccelerometer *)uiAccelerometer didAccelerate:(UIAcceleration *)acceleration
{
    accelerometer.Update( vec3f( acceleration.x, acceleration.y, acceleration.z ) );
}

- (void)render
{    
    glClearColor( 0, 0, 0, 1 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    // render floor

    {
        glUseProgram( _floorProgram );
                
        [opengl selectTexturedMesh:_floorTexture vertexBuffer:_floorVertexBuffer indexBuffer:_floorIndexBuffer];

        glUniformMatrix4fv( _floorUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&game.GetClipMatrix() );
        glUniformMatrix3fv( _floorUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&game.GetNormalMatrix() );
        glUniform3fv( _floorUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.GetLightPosition() );

        glDrawElements( GL_TRIANGLES, _floorMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
    }

    // render board

    {
        glUseProgram( _boardProgram );
        
        [opengl selectTexturedMesh:_boardTexture vertexBuffer:_boardVertexBuffer indexBuffer:_boardIndexBuffer];

        glUniformMatrix4fv( _boardUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&game.GetClipMatrix() );
        glUniformMatrix3fv( _boardUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&game.GetNormalMatrix() );
        glUniform3fv( _boardUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.GetLightPosition() );

        glDrawElements( GL_TRIANGLES, _boardMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
    }

    const vec3f & lightPosition = game.GetLightPosition();

#if SHADOWS

    // render board shadow on ground
        
    {
        glUseProgram( _shadowProgram );
        
        [opengl selectTexturedMesh:0 vertexBuffer:_boardVertexBuffer indexBuffer:_boardIndexBuffer];

        float boardShadowAlpha = 1.0f;

        mat4f shadowMatrix;
        MakeShadowMatrix( vec4f(0,0,1,-0.01f), vec4f( lightPosition.x(), lightPosition.y(), lightPosition.z(), 1 ), shadowMatrix );

        mat4f modelView = game.GetCameraMatrix() * shadowMatrix;
        mat4f modelViewProjectionMatrix = game.GetProjectionMatrix() * modelView;
        
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

        const std::vector<StoneInstance> & stones = game.GetStones();
        
        for ( int i = 0; i < stones.size(); ++i )
        {
            const StoneInstance & stone = stones[i];
            
            mat4f shadowMatrix;
            MakeShadowMatrix( vec4f(0,0,1,0), vec4f( lightPosition.x(), lightPosition.y(), lightPosition.z(), 1 ), shadowMatrix );

            mat4f modelView = game.GetCameraMatrix() * shadowMatrix * stone.visualTransform;
            mat4f modelViewProjectionMatrix = game.GetProjectionMatrix() * modelView;
            
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

        const std::vector<StoneInstance> & stones = game.GetStones();

        for ( int i = 0; i < stones.size(); ++i )
        {
            const StoneInstance & stone = stones[i];

            if ( stone.rigidBody.position.z() < game.GetBoard().GetThickness() )
                continue;

            mat4f shadowMatrix;
            MakeShadowMatrix( vec4f(0,0,1,-game.GetBoard().GetThickness()+0.1f), vec4f( lightPosition.x(), lightPosition.y(), lightPosition.z(), 1 ), shadowMatrix );
            
            mat4f modelView = game.GetCameraMatrix() * shadowMatrix * stone.visualTransform;
            mat4f modelViewProjectionMatrix = game.GetProjectionMatrix() * modelView;
            
            glUniformMatrix4fv( _shadowUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&modelViewProjectionMatrix );
            
            float shadowAlpha = GetShadowAlpha( stone.rigidBody.position );
            glUniform1fv( _shadowUniforms[UNIFORM_ALPHA], 1, (float*)&shadowAlpha );

            glDrawElements( GL_TRIANGLES, _stoneMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        }

        glDepthFunc( GL_LESS );

        glDisable( GL_BLEND );
    }

#endif

    // render white stones

    {
        glUseProgram( _stoneProgramWhite );
                
        [opengl selectNonTexturedMesh:_stoneVertexBuffer indexBuffer:_stoneIndexBuffer];

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

    // render black stones
    
    {
        glUseProgram( _stoneProgramBlack );
        
        [opengl selectNonTexturedMesh:_stoneVertexBuffer indexBuffer:_stoneIndexBuffer];
        
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

    // *** IMPORTANT: RENDER ALPHA BLENDED OBJECTS PAST HERE ***

    // render grid
    
    {
        glUseProgram( _gridProgram );

        [opengl selectTexturedMesh:_lineTexture vertexBuffer:_gridVertexBuffer indexBuffer:_gridIndexBuffer];
        
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
        
        [opengl selectTexturedMesh:_pointTexture vertexBuffer:_pointVertexBuffer indexBuffer:_pointIndexBuffer];

        glUniformMatrix4fv( _pointUniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, (float*)&game.GetClipMatrix() );
        glUniformMatrix3fv( _pointUniforms[UNIFORM_NORMAL_MATRIX], 1, 0, (float*)&game.GetNormalMatrix() );
        glUniform3fv( _pointUniforms[UNIFORM_LIGHT_POSITION], 1, (float*)&game.GetLightPosition() );
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_ZERO, GL_SRC_COLOR );

        glDrawElements( GL_TRIANGLES, _pointMesh.GetNumTriangles()*3, GL_UNSIGNED_SHORT, NULL );
        
        glDisable( GL_BLEND );
    }


    const GLenum discards[]  = { GL_DEPTH_ATTACHMENT };
    glDiscardFramebufferEXT( GL_FRAMEBUFFER, 1, discards );
}

- (void)update
{
    float dt = 1.0f / 60.0f;

    game.Update( dt );

    [self render];

    telemetry.Update( dt, game.GetBoard(), game.GetStones(), game.IsLocked(), accelerometer.GetUp() );
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    // IMPORTANT: otherwise we may not have correct matrices
    game.UpdateCamera();

    [self render];
}

@end
