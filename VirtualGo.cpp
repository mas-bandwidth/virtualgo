/*
    Virtual Go GDC 2013 Demo
    Copyright (c) 2005-2013, Glenn Fiedler. All rights reserved.
*/

#include "Common.h"
#include "Render.h"
#include "Stone.h"
#include "Platform.h"
#include "Biconvex.h"
#include "RigidBody.h"
#include "CollisionDetection.h"
#include "CollisionResponse.h"
#include "Intersection.h"
#include "stb_image.h"

using namespace platform;

const float FOV = 40.0f;
const float Near = 0.1f;
const float Far = 100.0f;

const int MaxZoomLevel = 5;

int zoomLevel = 2;

const float ScrollSpeed = 10.0f;

float scrollX = 0;
float scrollY = 0;
float scrollZ = 0;

enum Mode
{
    Nothing,
    LinearCollisionResponse,
    AngularCollisionResponse,
    CollisionResponseWithFriction,
    SolidColor,
    Textured,
    NumModes
};

Mode mode = Nothing;

Mode cameraMode = mode;
vec3f cameraLookAt;
vec3f cameraPosition;

const int DefaultBoardSize = 9;
const float MinimumBoardThickness = 0.2f;
const float DefaultBoardThickness = 0.5f;
const float MaximumBoardThickness = 9.0f;

Board board( DefaultBoardSize );

enum StoneDropType
{
    STONE_DROP_Horizontal,
    STONE_DROP_CCW45Degrees,
    STONE_DROP_Vertical,
    STONE_DROP_CW45Degrees,
    STONE_DROP_RandomNoSpin,
    STONE_DROP_RandomWithSpin
};

StoneDropType stoneDropType = STONE_DROP_RandomNoSpin;

Stone stone;
StoneSize size = STONE_SIZE_34;
Stone stoneSizes[STONE_SIZE_NumValues];

void RandomStone( const Biconvex & biconvex, RigidBody & rigidBody, Mode mode )
{
    const float x = scrollX;
    const float z = scrollZ;

    rigidBody.position = vec3f( x, board.GetThickness() + 15.0f, z );
    rigidBody.linearMomentum = vec3f(0,0,0);

    if ( stoneDropType == STONE_DROP_Horizontal || mode == LinearCollisionResponse )
    {
        rigidBody.orientation = quat4f(1,0,0,0);
        rigidBody.angularMomentum = vec3f(0,0,0);
    }
    else if ( stoneDropType == STONE_DROP_CCW45Degrees )
    {
        rigidBody.orientation = quat4f::axisRotation( -0.25f * pi, vec3f(0,0,-1) );
        rigidBody.angularMomentum = vec3f(0,0,0);
    }
    else if ( stoneDropType == STONE_DROP_Vertical )
    {
        rigidBody.orientation = quat4f::axisRotation( 0.5f * pi, vec3f(0,0,-1) );
        rigidBody.angularMomentum = vec3f(0,0,0);
    }
    else if ( stoneDropType == STONE_DROP_CW45Degrees )
    {
        rigidBody.orientation = quat4f::axisRotation( 0.25f * pi, vec3f(0,0,-1) );
        rigidBody.angularMomentum = vec3f(0,0,0);
    }
    else if ( stoneDropType == STONE_DROP_RandomNoSpin )
    {
        rigidBody.orientation = quat4f::axisRotation( random_float(0,2*pi), vec3f( random_float(0.1f,1), random_float(0.1f,1), random_float(0.1f,1) ) );
        rigidBody.angularMomentum = vec3f(0,0,0);
    }
    else if ( stoneDropType == STONE_DROP_RandomWithSpin )
    {
        rigidBody.orientation = quat4f::axisRotation( random_float(0,2*pi), vec3f( random_float(0.1f,1), random_float(0.1f,1), random_float(0.1f,1) ) );
    }

    rigidBody.Update();
}

void SelectStoneSize( int newStoneSize )
{
    size = (StoneSize) clamp( newStoneSize, 0, STONE_SIZE_NumValues - 1 );
    stone.biconvex = stoneSizes[size].biconvex;
    stone.rigidBody.mass = stoneSizes[size].rigidBody.mass;
    stone.rigidBody.inverseMass = stoneSizes[size].rigidBody.inverseMass;
    stone.rigidBody.inertia = stoneSizes[size].rigidBody.inertia;
    stone.rigidBody.inertiaTensor = stoneSizes[size].rigidBody.inertiaTensor;
    stone.rigidBody.inverseInertiaTensor = stoneSizes[size].rigidBody.inverseInertiaTensor;
}

void RestoreDefaults()
{
    SelectStoneSize( STONE_SIZE_34 );

    board.SetThickness( DefaultBoardThickness );

    scrollX = 0;
    scrollY = 0;
    scrollZ = 0;

    zoomLevel = 3;

    stone.rigidBody.position = vec3f( 0, board.GetThickness() + stone.biconvex.GetHeight() / 2, 0 );
    stone.rigidBody.orientation = quat4f(1,0,0,0);
    stone.rigidBody.linearMomentum = vec3f(0,0,0);
    stone.rigidBody.angularMomentum = vec3f(0,0,0);
    stone.rigidBody.Update();

    board = Board( DefaultBoardSize );

    stoneDropType = STONE_DROP_RandomWithSpin;    
}

void CheckOpenGLError( const char * message )
{
    int error = glGetError();
    if ( error != GL_NO_ERROR )
    {
        printf( "opengl error: %s (%s)\n", gluErrorString( error ), message );
        exit( 1 );
    }    
}

int main()
{
    printf( "[virtual go]\n" );

    // load textures

    int texture_width, texture_height, texture_n;
    const char filename[] = "textures/murasaki.jpg";
    printf( "loading %s\n", filename );
    unsigned char * texture_data = stbi_load( filename, &texture_width, &texture_height, &texture_n, 3 );
    if ( !texture_data )
    {
        printf( "failed to load: %s\n", filename );
        exit(1);
    }

    // initialize stones

    printf( "tesselating go stones...\n" );

    for ( int i = 0; i < STONE_SIZE_NumValues; ++i )
        stoneSizes[i].Initialize( (StoneSize)i );
    
    Mesh mesh[STONE_SIZE_NumValues];
    for ( int i = 0; i < STONE_SIZE_NumValues; ++i )
        GenerateBiconvexMesh( mesh[i], stoneSizes[i].biconvex );

    int displayWidth, displayHeight;
    GetDisplayResolution( displayWidth, displayHeight );

    #ifdef LETTERBOX
    displayWidth = 1280;
    displayHeight = 800;
    #endif

    printf( "opening display: %d x %d\n", displayWidth, displayHeight );

    if ( !OpenDisplay( "Virtual Go", displayWidth, displayHeight, 32 ) )
    {
        printf( "error: failed to open display" );
        return 1;
    }

    CheckOpenGLError( "after display open" );

    HideMouseCursor();

    // create opengl textures

    GLuint textureId;

    glGenTextures( 1, &textureId );

    glBindTexture( GL_TEXTURE_2D, textureId );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data );

    CheckOpenGLError( "after texture create" );

    // setup opengl

    glEnable( GL_LINE_SMOOTH );
    glEnable( GL_POLYGON_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

    GLfloat light_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };

    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    glLightModeli( GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR );

    glLightfv( GL_LIGHT0, GL_AMBIENT, light_ambient );
    glLightfv( GL_LIGHT0, GL_DIFFUSE, light_diffuse );
    glLightfv( GL_LIGHT0, GL_SPECULAR, light_specular );

    glLightfv( GL_LIGHT1, GL_AMBIENT, light_ambient );
    glLightfv( GL_LIGHT1, GL_DIFFUSE, light_diffuse );
    glLightfv( GL_LIGHT1, GL_SPECULAR, light_specular );

    glLightfv( GL_LIGHT2, GL_AMBIENT, light_ambient );
    glLightfv( GL_LIGHT2, GL_DIFFUSE, light_diffuse );
    glLightfv( GL_LIGHT2, GL_SPECULAR, light_specular );

    glLightfv( GL_LIGHT3, GL_AMBIENT, light_ambient );
    glLightfv( GL_LIGHT3, GL_DIFFUSE, light_diffuse );
    glLightfv( GL_LIGHT3, GL_SPECULAR, light_specular );

    GLfloat radiosity_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat radiosity_diffuse[] = { 0.3, 0.17, 0.02, 1.0 };
    GLfloat radiosity_specular[] = { 0,0,0,0 };

    glLightfv( GL_LIGHT4, GL_AMBIENT, radiosity_ambient );
    glLightfv( GL_LIGHT4, GL_DIFFUSE, radiosity_diffuse );
    glLightfv( GL_LIGHT4, GL_SPECULAR, radiosity_specular );

    glShadeModel( GL_SMOOTH );

    GLfloat lightAmbientColor[] = { 0.2, 0.2, 0.2, 1.0 };
    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lightAmbientColor );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    CheckOpenGLError( "after opengl setup" );

    bool quit = false;

    srand( time( NULL ) );

    const float normal_dt = 1.0f / 60.0f;
    const float slowmo_dt = normal_dt * 0.1f;

    float dt = normal_dt;

    uint64_t frame = 0;

    bool prevOne = false;
    bool prevTwo = false;
    bool prevThree = false;
    bool prevFour = false;
    bool prevFive = false;
    bool prevSpace = false;
    bool prevEnter = false;
    bool prevAltCtrlUp = false;
    bool prevAltCtrlDown = false;
    bool slowmo = false;

    RestoreDefaults();

    while ( !quit )
    {
        CheckOpenGLError( "frame start" );

        UpdateEvents();

        platform::Input input;
        
        input = platform::Input::Sample();

        if ( input.quit )
            quit = true;

        if ( input.escape )
            RestoreDefaults();

        if ( input.space && !prevSpace )
        {
            slowmo = !slowmo;
        }
        prevSpace = input.space;

        if ( input.enter && !prevEnter )
        {
            RandomStone( stone.biconvex, stone.rigidBody, mode );
            dt = normal_dt;
            slowmo = false;
        }
        prevEnter = input.enter;

        if ( input.tab )
        {
            scrollX += ( stone.rigidBody.position.x() - scrollX ) * 0.075f;
            scrollY += ( stone.rigidBody.position.y() - scrollY ) * 0.075f;
            scrollZ += ( stone.rigidBody.position.z() - scrollZ ) * 0.075f;
        }

        if ( input.f1 )
            stoneDropType = STONE_DROP_Horizontal;
        else if ( input.f2 )
            stoneDropType = STONE_DROP_CCW45Degrees;
        else if ( input.f3 )
            stoneDropType = STONE_DROP_Vertical;
        else if ( input.f4 )
            stoneDropType = STONE_DROP_CW45Degrees;
        else if ( input.f5 )
            stoneDropType = STONE_DROP_RandomNoSpin;
        else if ( input.f6 )
            stoneDropType = STONE_DROP_RandomWithSpin;

        const float Spin = 0.1f;

        bool appliedSpin = false;

        if ( input.q )
        {
            stone.rigidBody.angularMomentum += Spin * vec3f(1,0,0);
            appliedSpin = true;
        }

        if ( input.w )
        {
            stone.rigidBody.angularMomentum += Spin * vec3f(0,1,0);
            appliedSpin = true;
        }

        if ( input.e )
        {
            stone.rigidBody.angularMomentum += Spin * vec3f(0,0,1);
            appliedSpin = true;
        }

        if ( input.r )
        {
            stone.rigidBody.angularMomentum *= 0.75f;
            appliedSpin = true;
        }

        if ( input.alt && input.control )
        {
            if ( input.up && !prevAltCtrlUp )
            {
                zoomLevel --;
                if ( zoomLevel < 0 )
                    zoomLevel = 0;
            }
            prevAltCtrlUp = input.up;

            if ( input.down && !prevAltCtrlDown )
            {
                zoomLevel ++;
                if ( zoomLevel > MaxZoomLevel - 1 )
                    zoomLevel = MaxZoomLevel - 1;
            }
            prevAltCtrlDown = input.down;
        }
        else if ( input.alt )
        {
            if ( input.left )
                SelectStoneSize( size - 1 );
            
            if ( input.right )
                SelectStoneSize( size + 1 );

            if ( input.down )
            {
                float thickness = board.GetThickness();
                thickness *= 0.75f;
                if ( thickness < MinimumBoardThickness )
                    thickness = MinimumBoardThickness;
                board.SetThickness( thickness );
            }

            if ( input.up )
            {
                float thickness = board.GetThickness();
                thickness *= 1.25f;
                if ( thickness > MaximumBoardThickness )
                    thickness = MaximumBoardThickness;
                board.SetThickness( thickness );
            }

            if ( input.one )
                board = Board( 9 );

            if ( input.two )
                board = Board( 13 );

            if ( input.three )
                board = Board( 19 );

            prevAltCtrlUp = false;
            prevAltCtrlDown = false;
        }
        else if ( input.control )
        {
            const float Slide = 50;

            if ( input.left )
                stone.rigidBody.linearMomentum += Slide * vec3f(-1,0,0) * dt;

            if ( input.right )
                stone.rigidBody.linearMomentum += Slide * vec3f(+1,0,0) * dt;

            if ( input.up )
                stone.rigidBody.linearMomentum += Slide * vec3f(0,0,1) * dt;

            if ( input.down )
                stone.rigidBody.linearMomentum += Slide * vec3f(0,0,-1) * dt;

            prevAltCtrlUp = false;
            prevAltCtrlDown = false;
        }
        else
        {
            float sx = 0;
            float sy = 0;
            float sz = 0;

            if ( input.left )
                sx = -1;

            if ( input.right )
                sx = 1;

            if ( input.up )
                sz = 1;

            if ( input.down )
                sz = -1;

            if ( input.a )
                sy = 1;

            if ( input.z )
                sy = -1;

            vec3f scroll(sx,sy,sz);

            if ( length_squared( scroll ) > 0 )
            {
                scroll = normalize( scroll ) * ScrollSpeed * normal_dt;
                scrollX += scroll.x();
                scrollY += scroll.y();
                scrollZ += scroll.z();

                if ( scrollY > 50 )
                    scrollY = 50;
            }

            if ( input.one && !prevOne )
            {
                mode = LinearCollisionResponse;
                dt = normal_dt;
                slowmo = false;
            }
            prevOne = input.one;

            if ( input.two && !prevTwo )
            {
                mode = AngularCollisionResponse;
                dt = normal_dt;
                slowmo = false;
            }
            prevTwo = input.two;

            if ( input.three && !prevThree )
            {
                mode = CollisionResponseWithFriction;
                dt = normal_dt;
                slowmo = false;
            }
            prevThree = input.three;

            if ( input.four && !prevFour )
            {
                mode = SolidColor;
                dt = normal_dt;
                slowmo = false;
            }
            prevFour = input.four;

            if ( input.five && !prevFive )
            {
                mode = Textured;
                dt = normal_dt;
                slowmo = false;
            }
            prevFive = input.five;

            prevAltCtrlUp = false;
            prevAltCtrlDown = false;
        }

        glDepthMask( GL_TRUE );

        if ( mode >= SolidColor )
            ClearScreen( displayWidth, displayHeight, 0.15f, 0.15f, 0.15f );
        else
            ClearScreen( displayWidth, displayHeight );

        if ( mode == Nothing )
        {
            UpdateDisplay( 1 );
            frame++;
            continue;
        }
        // setup lights

        GLfloat lightPosition0[] = { 250, 1000, -500, 1 };
        GLfloat lightPosition1[] = { -250, 0, -250, 1 };
        GLfloat lightPosition2[] = { 100, 0, -100, 1 };
        GLfloat lightPosition3[] = { 0, +1000, +1000, 1 };
        GLfloat lightPosition4[] = { 0, -1000, 0, 1 };
        
        glLightfv( GL_LIGHT0, GL_POSITION, lightPosition0 );
        glLightfv( GL_LIGHT1, GL_POSITION, lightPosition1 );
        glLightfv( GL_LIGHT2, GL_POSITION, lightPosition2 );
        glLightfv( GL_LIGHT3, GL_POSITION, lightPosition3 );
        glLightfv( GL_LIGHT4, GL_POSITION, lightPosition4 );

        // setup projection + modelview

        glMatrixMode( GL_PROJECTION );

        glLoadIdentity();
        
        float flipX[] = { -1,0,0,0,
                           0,1,0,0,
                           0,0,1,0,
                           0,0,0,1 };
        
        glMultMatrixf( flipX );

        gluPerspective( FOV, (float) displayWidth / (float) displayHeight, Near, Far );

        glMatrixMode( GL_MODELVIEW );    

        glLoadIdentity();

        // update camera

        {
            const float x = scrollX;
            const float y = scrollY;
            const float z = scrollZ;

            vec3f targetLookAt;
            vec3f targetPosition;

            const float t = board.GetThickness();

            if ( zoomLevel == 0 )
            {
                targetLookAt = vec3f(x,y,z);
                targetPosition = vec3f(x,y,z-5);
            }
            else if ( zoomLevel == 1 )
            {
                targetLookAt = vec3f(x,y+1,z);
                targetPosition = vec3f(x,y+1,z-10);
            }
            else if ( zoomLevel == 2 )
            {
                targetLookAt = vec3f(x,y+4,z);
                targetPosition = vec3f(x,y+8,z-25);
            }
            else if ( zoomLevel == 3 )
            {
                targetLookAt = vec3f(x,y+2,z);
                targetPosition = vec3f(x,y+20,z-30);
            }
            else if ( zoomLevel == 4 )
            {
                targetLookAt = vec3f(x,y,z);
                targetPosition = vec3f(x,y+55,z);
            }

            if ( cameraMode == mode )
            {
                cameraLookAt += ( targetLookAt - cameraLookAt ) * 0.25f;
                cameraPosition += ( targetPosition - cameraPosition ) * 0.25f;
            }
            else
            {
                cameraLookAt = targetLookAt;
                cameraPosition = targetPosition;
            }

            float amountBelow = t - fmax( targetPosition.y(), targetLookAt.y() );
            if ( amountBelow > 0 )
                scrollY += amountBelow;

            cameraMode = mode;

            vec3f cameraUp = cross( normalize( cameraLookAt - cameraPosition ), vec3f(1,0,0) );

            gluLookAt( cameraPosition.x(), max( cameraPosition.y(), t ), cameraPosition.z(),
                       cameraLookAt.x(), max( cameraLookAt.y(), t ), cameraLookAt.z(),
                       cameraUp.x(), cameraUp.y(), cameraUp.z() );
        }

        // if the stone is inside the board at the beginning of the frame
        // push it out with a spring impulse -- this gives a cool effect
        // when the board thickness is adjusted dynamically (trapoline)

        {
            StaticContact boardContact;
            if ( StoneBoardCollision( stone.biconvex, board, stone.rigidBody, boardContact ) )
                stone.rigidBody.linearMomentum += boardContact.normal * boardContact.depth * 10;
        }

        // update stone physics

        const float target_dt = slowmo ? slowmo_dt : normal_dt;
        const float tightness = ( target_dt < dt ) ? 0.2f : 0.1f;
        dt += ( target_dt - dt ) * tightness;

        const int iterations = 20;

        const float iteration_dt = dt / iterations;

        for ( int i = 0; i < iterations; ++i )
        {
            const float gravity = 9.8f * 10;    // cms/sec^2
            stone.rigidBody.linearMomentum += vec3f(0,-gravity,0) * stone.rigidBody.mass * iteration_dt;

            stone.rigidBody.Update();

            stone.rigidBody.position += stone.rigidBody.linearVelocity * iteration_dt;
            quat4f spin = AngularVelocityToSpin( stone.rigidBody.orientation, stone.rigidBody.angularVelocity );
            stone.rigidBody.orientation += spin * iteration_dt;
            stone.rigidBody.orientation = normalize( stone.rigidBody.orientation );

            // collision between stone and board

            bool collided = false;

            const float board_e = 0.85f;
            const float board_u = 0.15f;

            StaticContact boardContact;
            if ( StoneBoardCollision( stone.biconvex, board, stone.rigidBody, boardContact ) )
            {
                if ( mode == LinearCollisionResponse )
                    ApplyLinearCollisionImpulse( boardContact, board_e );
                else if ( mode == AngularCollisionResponse )
                    ApplyCollisionImpulseWithFriction( boardContact, board_e, 0.0f );
                else if ( mode >= CollisionResponseWithFriction )
                    ApplyCollisionImpulseWithFriction( boardContact, board_e, board_u );
                stone.rigidBody.Update();
                collided = true;
            }

            // collision between stone and floor

            const float floor_e = 0.5f;
            const float floor_u = 0.25f;

            StaticContact floorContact;
            if ( StoneFloorCollision( stone.biconvex, board, stone.rigidBody, floorContact ) )
            {
                if ( mode == LinearCollisionResponse )
                    ApplyLinearCollisionImpulse( floorContact, floor_e );
                else if ( mode == AngularCollisionResponse )
                    ApplyCollisionImpulseWithFriction( floorContact, floor_e, 0.0f );
                else if ( mode >= CollisionResponseWithFriction )
                    ApplyCollisionImpulseWithFriction( floorContact, floor_e, floor_u );
                stone.rigidBody.Update();
                collided = true;
            }

            // this is a *massive* hack to approximate rolling/spinning
            // friction and it is completely made up and not accurate at all!

            if ( collided )
            {
                float momentum = length( stone.rigidBody.angularMomentum );
                const float factor_a = 0.9925f;
                const float factor_b = 0.9995f;
                const float a = 0.0f;
                const float b = 1.0f;
                if ( momentum >= b || appliedSpin )
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

            stone.rigidBody.linearMomentum *= 0.99999f;
            stone.rigidBody.angularMomentum *= 0.99999f;
        }

        // setup lights for board

        glEnable( GL_LIGHT0 );
        glDisable( GL_LIGHT1 );
        glDisable( GL_LIGHT2 );
        glDisable( GL_LIGHT3 );
        glDisable( GL_LIGHT4 );
        
        // render board

        glDepthMask( GL_TRUE );

        {
            glColor4f( 0,0,0,1 );
            glDisable( GL_LIGHTING );
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    
            glDisable( GL_CULL_FACE );
    
            RenderBoard( board );
    
            glEnable( GL_CULL_FACE );
        }

        const float w = board.GetWidth() / 2;
        const float h = board.GetHeight() / 2;
        const float t = board.GetThickness();
        const float e = 0.001f;

        if ( cameraPosition.x() >= -w - e &&
             cameraPosition.x() <= +w + e &&
             cameraPosition.z() >= -h - e - Near &&
             cameraPosition.z() <= +h + e + Near &&
             cameraPosition.y() <= t + e )
        {
            glDisable( GL_DEPTH_TEST );

            glLineWidth( 5 );
            glBegin( GL_LINES );
            glVertex3f( -1000, board.GetThickness(), cameraPosition.z() + 0.11f );
            glVertex3f( +1000, board.GetThickness(), cameraPosition.z() + 0.11f );
            glEnd();

            glEnable( GL_DEPTH_TEST );
        }
        else
        {
            if ( mode >= SolidColor )
            {
                glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

                glEnable( GL_LIGHTING );

                GLfloat mat_ambient[] = { 1.0, 0.6, 0.1, 1.0 };
                GLfloat mat_diffuse[] = { 1.0, 0.6, 0.1, 1.0 };
                GLfloat mat_specular[] = { 0, 0, 0, 1.0 };
                GLfloat mat_shininess[] = { 50.0 };

                glMaterialfv( GL_FRONT, GL_AMBIENT, mat_ambient );
                glMaterialfv( GL_FRONT, GL_DIFFUSE, mat_diffuse );
                glMaterialfv( GL_FRONT, GL_SPECULAR, mat_specular );
                glMaterialfv( GL_FRONT, GL_SHININESS, mat_shininess );
            }
            else
            {
                glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
                glDepthMask( GL_FALSE );
                glLineWidth( 5 );
                glDisable( GL_LIGHTING );
                glColor4f( 0.4f,0.4f,0.4f,1 );
            }

            glDisable( GL_DEPTH_TEST );

            RenderBoard( board );

            // render grid

            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

            glDisable( GL_LIGHTING );

            glLineWidth( 3 );

            RenderGrid( board.GetThickness(), board.GetSize(), board.GetCellWidth(), board.GetCellHeight() );

            glEnable( GL_LIGHTING );

            glEnable( GL_DEPTH_TEST );
        }

        // setup lights for stone render

        glEnable( GL_LIGHT0 );
        glEnable( GL_LIGHT1 );
        glEnable( GL_LIGHT2 );
        glEnable( GL_LIGHT3 );

        if ( mode >= SolidColor )
            glEnable( GL_LIGHT4 );
        else
            glDisable( GL_LIGHT4 );

        // render stone

        glEnable( GL_LIGHTING );

        glPushMatrix();

        RigidBodyTransform biconvexTransform( stone.rigidBody.position, stone.rigidBody.orientation );
        float opengl_transform[16];
        biconvexTransform.localToWorld.store( opengl_transform );
        glMultMatrixf( opengl_transform );

        if ( mode < SolidColor )
        {
            glEnable( GL_BLEND ); 
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glLineWidth( 1 );
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
            glEnable( GL_DEPTH_TEST );
            glDepthMask( GL_FALSE );
        }
        else
        {
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
            glDepthMask( GL_FALSE );
            glEnable( GL_DEPTH_TEST );
            glDepthMask( GL_TRUE );
        }

        if ( mode >= Textured )
        {
            glBindTexture( GL_TEXTURE_2D, textureId );
            glEnable( GL_TEXTURE_2D );
        }
        else
            glColor4f( 1.0, 1.0, 1.0, 1 );

        GLfloat mat_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
        GLfloat mat_diffuse[] = { 0.4, 0.4, 0.4, 1.0 };
        GLfloat mat_specular[] = { 0.2, 0.2, 0.2, 1.0 };
        GLfloat mat_shininess[] = { 50.0 };

        glMaterialfv( GL_FRONT, GL_AMBIENT, mat_ambient );
        glMaterialfv( GL_FRONT, GL_DIFFUSE, mat_diffuse );
        glMaterialfv( GL_FRONT, GL_SPECULAR, mat_specular );
        glMaterialfv( GL_FRONT, GL_SHININESS, mat_shininess );

        RenderMesh( mesh[size] );

        glDisable( GL_TEXTURE_2D );

        glPopMatrix();

        // update the display
        
        UpdateDisplay( 1 );

        // update time

        frame++;

        CheckOpenGLError( "frame end" );
    }

    CloseDisplay();

    stbi_image_free( texture_data );

    return 0;
}
