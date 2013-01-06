/*
    Go stone tessellation demo.
    Copyright (c) 2004-2013, Glenn Fiedler. All rights reserved.
*/

#include "Common.h"
#include "Render.h"
#include "Stone.h"
#include "Mesh.h"
#include "Platform.h"
#include "Biconvex.h"
#include "RigidBody.h"
#include "Collision.h"
#include "Intersection.h"

using namespace platform;

static Biconvex biconvex( 2.2f, 1.13f );

static Biconvex biconvexWithBevel( 2.2f, 1.13f, 0.1f );

enum Mode
{
    Naive,
    Subdivision,
    SubdivisionWithBevel,
    Render
};

struct TessellationData
{
    Mode mode;
    int subdivisions;

    TessellationData()
    {
        mode = Naive;
        subdivisions = 5;
    }
};

TessellationData tessellation;

void UpdateTessellation( Mesh & mesh, Mode mode, int subdivisions )
{
    if ( mode == tessellation.mode && subdivisions == tessellation.subdivisions )
        return;

    mesh.Clear();

    if ( mode == SubdivisionWithBevel || mode == Render )
        GenerateBiconvexMesh( mesh, biconvexWithBevel, subdivisions );
    else
        GenerateBiconvexMesh( mesh, biconvex, subdivisions );

    tessellation.mode = mode;
    tessellation.subdivisions = subdivisions;
}

int main()
{
    srand( time( NULL ) );

    printf( "[tessellation]\n" );

    Mode mode = Naive;

    int subdivisions = 5;

    RigidBody rigidBody;
    rigidBody.mass = 1.0f;
    rigidBody.inverseMass = 1.0f / rigidBody.mass;
    CalculateBiconvexInertiaTensor( rigidBody.mass, biconvex, rigidBody.inertiaTensor, rigidBody.inverseInertiaTensor );

    Mesh mesh;
    
    int displayWidth, displayHeight;
    GetDisplayResolution( displayWidth, displayHeight );

    #ifdef LETTERBOX
    displayWidth = 1280;
    displayHeight = 800;
    #endif

    printf( "display resolution is %d x %d\n", displayWidth, displayHeight );

    if ( !OpenDisplay( "Tessellation Demo", displayWidth, displayHeight, 32 ) )
    {
        printf( "error: failed to open display" );
        return 1;
    }

    HideMouseCursor();

    // setup for render

    glEnable( GL_LINE_SMOOTH );
    glEnable( GL_POLYGON_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

    glShadeModel( GL_SMOOTH );

    GLfloat lightAmbientColor[] = { 0.75, 0.75, 0.75, 1.0 };
    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lightAmbientColor );

    glLightf( GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.01f );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glLineWidth( 4 );
    glColor4f( 0.5f,0.5f,0.5f,1 );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    mat4f rotation = mat4f::identity();

    bool prevLeft = false;
    bool prevRight = false;
    bool prevSpace = false;
    bool rotating = true;
    bool quit = false;

    float smoothedRotation = 0.0f;

    const float dt = 1.0f / 60.0f;

    uint64_t frame = 0;

    while ( true )
    {
        glPolygonMode( GL_FRONT_AND_BACK, mode != Render ? GL_LINE : GL_FILL );

        UpdateEvents();

        platform::Input input;
        
        input = platform::Input::Sample();

        if ( input.escape || input.quit )
            break;

        if ( input.one )
        {
            mode = Naive;
        }
        else if ( input.two )
        {
            mode = Subdivision;
        }
        else if ( input.three )
        {
            mode = SubdivisionWithBevel;
        }
        else if ( input.four )
        {
            mode = Render;
        }

        if ( input.space && !prevSpace )
            rotating = !rotating;
        prevSpace = input.space;

        if ( input.right && !prevRight )
        {
            subdivisions++;
            if ( subdivisions > 5 )
                subdivisions = 5;
        }
        prevRight = input.right;

        if ( input.left && !prevLeft )
        {
            subdivisions--;
            if ( subdivisions < 0 )
                subdivisions = 0;
        }
        prevLeft = input.left;

        UpdateTessellation( mesh, mode, subdivisions );

        ClearScreen( displayWidth, displayHeight );

        if ( frame > 20 )
        {
            const float fov = 25.0f;
            glMatrixMode( GL_PROJECTION );
            glLoadIdentity();
            gluPerspective( fov, (float) displayWidth / (float) displayHeight, 0.1f, 100.0f );

            float flipX[] = { -1,0,0,0,
                               0,1,0,0,
                               0,0,1,0,
                               0,0,0,1 };

            glMultMatrixf( flipX );

            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();
            gluLookAt( -5, 0, 0, 
                       0, 0, 0, 
                       0, 1, 0 );

            // set light position

            GLfloat lightPosition[] = { -10, 3, 10, 1 };
            glLightfv( GL_LIGHT0, GL_POSITION, lightPosition );

            // render stone

            const float targetRotation = rotating ? 0.28f : 0.0f;
            smoothedRotation += ( targetRotation - smoothedRotation ) * 0.15f;
            mat4f deltaRotation = mat4f::axisRotation( smoothedRotation, vec3f(1,-2,3) );
            rotation = rotation * deltaRotation;

            RigidBodyTransform biconvexTransform( vec3f(0,0,0), rotation );

            glPushMatrix();

            float opengl_transform[16];
            biconvexTransform.localToWorld.store( opengl_transform );
            glMultMatrixf( opengl_transform );

            if ( mode == Naive )
            {
                float i = pow( subdivisions, 1.5f );
                int numSegments = 15 * i;
                int numRings = 1.75f * i + 1;
                if ( numSegments < 5 )
                    numSegments = 5;
                if ( numRings < 1 )
                    numRings = 1;
                RenderBiconvexNaive( biconvex, numSegments, numRings );
            }
            else
                RenderMesh( mesh );

            glPopMatrix();
        }

        // update the display
        
        UpdateDisplay( 1 );

        // update time

        frame++;
    }

    CloseDisplay();

    return 0;
}
