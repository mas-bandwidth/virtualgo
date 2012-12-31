/*
    Virtual Go
    A networked physics simulation of a go board and stones
    Copyright (c) 2005-2012, Glenn Fiedler. All rights reserved.
*/

#include "Common.h"
#include "Render.h"
#include "Stone.h"
#include "Platform.h"
#include "Biconvex.h"
#include "RigidBody.h"
#include "Collision.h"
#include "Intersection.h"

using namespace platform;

void ClearScreen( int displayWidth, int displayHeight )
{
    glViewport( 0, 0, displayWidth, displayHeight );
    glDisable( GL_SCISSOR_TEST );
    glClearStencil( 0 );
    // HACK: black clear color
    glClearColor( 0, 0, 0, 1 );//1.0f, 1.0f, 1.0f, 1.0f );     
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}

void RenderBiconvex( const Biconvex & biconvex, int numSegments = 128, int numRings = 32 )
{
    glBegin( GL_QUADS );

    const float sphereRadius = biconvex.GetSphereRadius();
    const float segmentAngle = 2*pi / numSegments;

    // top surface of biconvex
    {
        const float center_y = -biconvex.GetSphereOffset();
        const float delta_y = biconvex.GetHeight() / 2 / numRings;

        for ( int i = 0; i < numRings; ++i )
        {
            const float y1 = i * delta_y;
            const float s1 = y1 - center_y;
            const float r1 = sqrt( sphereRadius*sphereRadius - (s1*s1) );
            const float y2 = (i+1) * delta_y;
            const float s2 = y2 - center_y;
            const float r2 = sqrt( sphereRadius*sphereRadius - (s2*s2) );

            for ( int j = 0; j < numSegments; ++j )
            {
                const float angle1 = j * segmentAngle;
                const float angle2 = angle1 + segmentAngle;
                
                const float top_x1 = cos( angle1 ) * r1;
                const float top_z1 = sin( angle1 ) * r1;    
                const float top_x2 = cos( angle2 ) * r1;
                const float top_z2 = sin( angle2 ) * r1;

                const float bottom_x1 = cos( angle1 ) * r2;
                const float bottom_z1 = sin( angle1 ) * r2;    
                const float bottom_x2 = cos( angle2 ) * r2;
                const float bottom_z2 = sin( angle2 ) * r2;

                vec3f a( bottom_x1, y2, bottom_z1 );
                vec3f b( top_x1, y1, top_z1 );
                vec3f c( top_x2, y1, top_z2 );
                vec3f d( bottom_x2, y2, bottom_z2 );

                vec3f center( 0, center_y, 0 );

                vec3f na = normalize( a - center );
                vec3f nb = normalize( b - center );
                vec3f nc = normalize( c - center );
                vec3f nd = normalize( d - center );

                glNormal3f( na.x(), na.y(), na.z() );
                glVertex3f( a.x(), a.y(), a.z() );
                
                glNormal3f( nb.x(), nb.y(), nb.z() );
                glVertex3f( b.x(), b.y(), b.z() );

                glNormal3f( nc.x(), nc.y(), nc.z() );
                glVertex3f( c.x(), c.y(), c.z() );
                
                glNormal3f( nd.x(), nd.y(), nd.z() );
                glVertex3f( d.x(), d.y(), d.z() );
            }
        }
    }

    // bottom surface of biconvex
    {
        const float center_y = biconvex.GetSphereOffset();
        const float delta_y = biconvex.GetHeight() / 2 / numRings;

        for ( int i = 0; i < numRings; ++i )
        {
            const float y1 = -i * delta_y;
            const float s1 = y1 - center_y;
            const float r1 = sqrt( sphereRadius*sphereRadius - (s1*s1) );
            const float y2 = -(i+1) * delta_y;
            const float s2 = y2 - center_y;
            const float r2 = sqrt( sphereRadius*sphereRadius - (s2*s2) );

            for ( int j = 0; j < numSegments; ++j )
            {
                const float angle1 = j * segmentAngle;
                const float angle2 = angle1 + segmentAngle;
                
                const float top_x1 = cos( angle1 ) * r1;
                const float top_z1 = sin( angle1 ) * r1;    
                const float top_x2 = cos( angle2 ) * r1;
                const float top_z2 = sin( angle2 ) * r1;

                const float bottom_x1 = cos( angle1 ) * r2;
                const float bottom_z1 = sin( angle1 ) * r2;    
                const float bottom_x2 = cos( angle2 ) * r2;
                const float bottom_z2 = sin( angle2 ) * r2;

                vec3f a( top_x1, y1, top_z1 );
                vec3f b( bottom_x1, y2, bottom_z1 );
                vec3f c( bottom_x2, y2, bottom_z2 );
                vec3f d( top_x2, y1, top_z2 );

                vec3f center( 0, center_y, 0 );

                vec3f na = normalize( a - center );
                vec3f nb = normalize( b - center );
                vec3f nc = normalize( c - center );
                vec3f nd = normalize( d - center );

                glNormal3f( na.x(), na.y(), na.z() );
                glVertex3f( a.x(), a.y(), a.z() );
                
                glNormal3f( nb.x(), nb.y(), nb.z() );
                glVertex3f( b.x(), b.y(), b.z() );

                glNormal3f( nc.x(), nc.y(), nc.z() );
                glVertex3f( c.x(), c.y(), c.z() );
                
                glNormal3f( nd.x(), nd.y(), nd.z() );
                glVertex3f( d.x(), d.y(), d.z() );
            }
        }
    }

    glEnd();
}

void SubdivideBiconvexMesh( const Biconvex & biconvex, 
                            bool i, bool j, bool k,
                            vec3f a, vec3f b, vec3f c,
                            vec3f an, vec3f bn, vec3f cn,
                            vec3f sphereCenter,
                            int depth, int subdivisions )
{
    // edges: i = c -> a
    //        j = a -> b
    //        k = b -> c

    // vertices:
    //
    //        a
    //
    //     e     d 
    //
    //  b     f     c

    if ( depth < subdivisions )
    {
        vec3f d = ( a + c ) / 2;
        vec3f e = ( a + b ) / 2;
        vec3f f = ( b + c ) / 2;

        vec3f dn, en, fn;

        const float circleRadius = biconvex.GetCircleRadius();
        const float sphereRadius = biconvex.GetSphereRadius();

        if ( i )
        {
            d = normalize( d ) * circleRadius;
            dn = normalize( d - sphereCenter );
        }
        else
        {
            dn = normalize( d - sphereCenter );
            d = sphereCenter + dn * sphereRadius;
        }

        if ( j )
        {
            e = normalize( e ) * circleRadius;
            en = normalize( e - sphereCenter );
        }
        else
        {
            en = normalize( e - sphereCenter );
            e = sphereCenter + en * sphereRadius;
        }

        if ( k )
        {
            f = normalize( f ) * circleRadius;
            fn = normalize( f - sphereCenter );
        }
        else
        {
            fn = normalize( f - sphereCenter );
            f = sphereCenter + fn * sphereRadius;
        }

        depth++;

        // vertices:
        //
        //        a
        //
        //     e     d 
        //
        //  b     f     c

        SubdivideBiconvexMesh( biconvex, i, j, false, a, e, d, an, en, dn, sphereCenter, depth, subdivisions );
        SubdivideBiconvexMesh( biconvex, false, j, k, e, b, f, en, bn, fn, sphereCenter, depth, subdivisions );
        SubdivideBiconvexMesh( biconvex, i, false, k, d, f, c, dn, fn, cn, sphereCenter, depth, subdivisions );
        SubdivideBiconvexMesh( biconvex, false, false, false, d, e, f, dn, en, fn, sphereCenter, depth, subdivisions );
    }
    else
    {
        glNormal3f( an.x(), an.y(), an.z() );
        glVertex3f( a.x(), a.y(), a.z() );

        glNormal3f( bn.x(), bn.y(), bn.z() );
        glVertex3f( b.x(), b.y(), b.z() );

        glNormal3f( cn.x(), cn.y(), cn.z() );
        glVertex3f( c.x(), c.y(), c.z() );
    }
}

void GenerateBiconvexMesh( const Biconvex & biconvex, int subdivisions = 5 )
{
    // HACK: render with glBegin/glEnd first to verify -- then convert to vertex buffer
    glBegin( GL_TRIANGLES );

    const float circleRadius = biconvex.GetCircleRadius();

    const int numTriangles = 5;

    const float deltaAngle = 360.0f / numTriangles;

    // top

    for ( int i = 0; i < numTriangles; ++i )
    {
        mat4f r1 = mat4f::axisRotation( deltaAngle * i, vec3f(0,0,1) );
        mat4f r2 = mat4f::axisRotation( deltaAngle * ( i + 1 ), vec3f(0,0,1) );

        vec3f a = vec3f( 0, 0, -biconvex.GetSphereOffset() + biconvex.GetSphereRadius() );
        vec3f b = transformPoint( r1, vec3f( 0, circleRadius, 0 ) );
        vec3f c = transformPoint( r2, vec3f( 0, circleRadius, 0 ) );

        const vec3f sphereCenter = vec3f( 0, 0, -biconvex.GetSphereOffset() );

        vec3f an = normalize( a - sphereCenter );
        vec3f bn = normalize( b - sphereCenter );
        vec3f cn = normalize( c - sphereCenter );

        SubdivideBiconvexMesh( biconvex, false, false, true, a, b, c, an, bn, cn, sphereCenter, 0, subdivisions );
    }

    // bottom

    for ( int i = 0; i < numTriangles; ++i )
    {
        mat4f r1 = mat4f::axisRotation( deltaAngle * i, vec3f(0,0,1) );
        mat4f r2 = mat4f::axisRotation( deltaAngle * ( i + 1 ), vec3f(0,0,1) );

        vec3f a = vec3f( 0, 0, biconvex.GetSphereOffset() - biconvex.GetSphereRadius() );
        vec3f b = transformPoint( r1, vec3f( 0, circleRadius, 0 ) );
        vec3f c = transformPoint( r2, vec3f( 0, circleRadius, 0 ) );

        const vec3f sphereCenter = vec3f( 0, 0, biconvex.GetSphereOffset() );

        vec3f an = normalize( a - sphereCenter );
        vec3f bn = normalize( b - sphereCenter );
        vec3f cn = normalize( c - sphereCenter );

        SubdivideBiconvexMesh( biconvex, false, false, true, a, b, c, an, bn, cn, sphereCenter, 0, subdivisions );
    }

    glEnd();
}

void GetMousePickRay( int mouse_x, int mouse_y, vec3f & rayStart, vec3f & rayDirection )
{
    // IMPORTANT: discussion about various ways to convert mouse coordinates -> pick ray
    // http://stackoverflow.com/questions/2093096/implementing-ray-picking

    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    glGetIntegerv( GL_VIEWPORT, viewport );
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );

    const float displayHeight = viewport[3];

    float x = mouse_x;
    float y = displayHeight - mouse_y;

    GLdouble x1,y1,z1;
    GLdouble x2,y2,z2;

    gluUnProject( x, y, 0, modelview, projection, viewport, &x1, &y1, &z1 );
    gluUnProject( x, y, 1, modelview, projection, viewport, &x2, &y2, &z2 );

    vec3f ray1( x1,y1,z1 );
    vec3f ray2( x2,y2,z2 );

    rayStart = ray1;
    rayDirection = normalize( ray2 - ray1 );
}

void RenderBoard( const Board & board )
{
    glBegin( GL_QUADS );

    const float width = board.GetWidth();
    const float height = board.GetHeight();

    const float ideal = 1.0f;

    const int steps_x = (int) ceil( width / ideal ); 
    const int steps_y = (int) ceil( height / ideal );

    const float dx = width / steps_x;
    const float dy = height / steps_y;

    float x = - width / 2;

    for ( int i = 0; i < steps_x; ++i )
    {
        float y = - height / 2;

        for ( int j = 0; j < steps_x; ++j )
        {
            glVertex3f( x + dx, 0.0f, y );
            glVertex3f( x + dx, 0.0f, y + dy );
            glVertex3f( x, 0.0f, y + dy );
            glVertex3f( x, 0.0f, y );

            y += dy;
        }

        x += dx;
    }

    glEnd();
}

inline int random( int maximum )
{
    assert( maximum > 0 );
    int randomNumber = 0;
    randomNumber = rand() % maximum;
    return randomNumber;
}

inline float random_float( float min, float max )
{
    assert( max > min );
    return random( 1000000 ) / 1000000.f * ( max - min ) + min;
}

inline bool chance( float probability )
{
    assert( probability >= 0.0f );
    assert( probability <= 1.0f );
    const int percent = (int) ( probability * 100.0f );
    return random(100) <= percent;
}

enum Mode
{
    Stone,
    BiconvexSupport,
    StoneAndBoard,
    FallingStone,
    LinearCollisionResponse,
    AngularCollisionResponse,
    CollisionResponseWithFriction
};

void RandomStone( const Biconvex & biconvex, RigidBody & rigidBody, Mode mode )
{
    rigidBody.position = vec3f( 0, mode > LinearCollisionResponse ? 15.0f : 10.0f, 0 );
    rigidBody.orientation = quat4f::axisRotation( random_float(0,2*pi), vec3f( random_float(0.1f,1), random_float(0.1f,1), random_float(0.1f,1) ) );
    rigidBody.linearVelocity = vec3f(0,0,0);
    rigidBody.angularVelocity = vec3f(0,0,0);
}

int main()
{
    printf( "[virtual go]\n" );

    Mode mode = Stone;//CollisionResponseWithFriction;

    Biconvex biconvex( 2.2f, 1.13f );

    RigidBody rigidBody;
    rigidBody.mass = 1.0f;
    rigidBody.inverseMass = 1.0f / rigidBody.mass;
    CalculateBiconvexInertiaTensor( rigidBody.mass, biconvex, rigidBody.inertiaTensor, rigidBody.inverseInertiaTensor );

    RandomStone( biconvex, rigidBody, mode );

    // HACK
    //GenerateBiconvexMesh( biconvex );

    int displayWidth, displayHeight;
    GetDisplayResolution( displayWidth, displayHeight );

    #ifdef LETTERBOX
    displayWidth = 1280;
    displayHeight = 800;
    #endif

    printf( "display resolution is %d x %d\n", displayWidth, displayHeight );

    if ( !OpenDisplay( "Virtual Go", displayWidth, displayHeight, 32 ) )
    {
        printf( "error: failed to open display" );
        return 1;
    }

    ShowMouseCursor();

    glEnable( GL_LINE_SMOOTH );
    glEnable( GL_POLYGON_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

    mat4f rotation = mat4f::identity();

    double t = 0.0f;

    bool prevSpace = false;
    bool prevEnter = false;
    bool rotating = true;
    bool quit = false;

    float smoothedRotation = 0.0f;

    srand( time( NULL ) );

    bool prevCollision = false;

    const float dt = 1.0f / 60.0f;

    while ( !quit )
    {
        UpdateEvents();

        platform::Input input;
        
        input = platform::Input::Sample();

        if ( input.escape )
            quit = true;

        if ( input.space )
        {
            if ( !prevSpace )
            {
                if ( mode == Stone || mode == BiconvexSupport )
                    rotating = !rotating;

                if ( mode == FallingStone || mode == LinearCollisionResponse || mode == AngularCollisionResponse || mode == CollisionResponseWithFriction )
                    RandomStone( biconvex, rigidBody, mode );
            }

            prevSpace = true;
        }
        else
            prevSpace = false;

        if ( input.enter )
        {
            if ( !prevEnter )
                rigidBody.linearVelocity += vec3f( -2,0,0 );
            prevEnter = true;
        }
        else
            prevEnter = false;

        if ( input.one )
            mode = Stone;

        if ( input.two )
            mode = BiconvexSupport;

        if ( input.three )
            mode = StoneAndBoard;

        if ( input.four )
        {
            mode = FallingStone;
            RandomStone( biconvex, rigidBody, mode );
        }

        if ( input.five )
        {
            mode = LinearCollisionResponse;
            RandomStone( biconvex, rigidBody, mode );
        }

        if ( input.six )
        {
            mode = AngularCollisionResponse;
            RandomStone( biconvex, rigidBody, mode );
        }

        if ( input.seven )
        {
            mode = CollisionResponseWithFriction;
            RandomStone( biconvex, rigidBody, mode );
        }

        ClearScreen( displayWidth, displayHeight );

        if ( mode == Stone || mode == BiconvexSupport )
        {
            if ( mode == Stone )
            {
                const float fov = 25.0f;
                glMatrixMode( GL_PROJECTION );
                glLoadIdentity();
                gluPerspective( fov, (float) displayWidth / (float) displayHeight, 0.1f, 100.0f );
            }
            else
            {
                glMatrixMode( GL_PROJECTION );
                glLoadIdentity();
                glOrtho( -1.5, +1.5f, -1.0f, +1.0f, 0.1f, 100.0f );
            }

            float flipX[] = { -1,0,0,0,
                               0,1,0,0,
                               0,0,1,0,
                               0,0,0,1 };

            glMultMatrixf( flipX );

            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();
            gluLookAt( 0, 0, -5.0f, 
                       0, 0, 0, 
                       0, 1, 0 );

            // render stone

            const float targetRotation = rotating ? 0.28f : 0.0f;
            smoothedRotation += ( targetRotation - smoothedRotation ) * 0.15f;
            mat4f deltaRotation = mat4f::axisRotation( smoothedRotation, vec3f(1,2,3) );
            rotation = rotation * deltaRotation;

            RigidBodyTransform biconvexTransform( vec3f(0,0,0), rotation );


            // setup lights

            vec3f lightPosition( +10, +10, -10 );

            glEnable( GL_LIGHTING );
            glEnable( GL_LIGHT0 );

            glShadeModel( GL_SMOOTH );

            //GLfloat lightAmbientColor[] = { 0.5, 0.5, 0.5, 1.0 };
            //glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lightAmbientColor );

            //glLightf( GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.01f );

            GLfloat position[4];
            position[0] = lightPosition.x();
            position[1] = lightPosition.y();
            position[2] = lightPosition.z();
            position[3] = 1.0f;
            glLightfv( GL_LIGHT0, GL_POSITION, position );


            glEnable( GL_BLEND ); 
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            //glLineWidth( 1 );
            //glColor4f( 0.5f,0.5f,0.5f,1 );
            glEnable( GL_CULL_FACE );
            glCullFace( GL_BACK );
            //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );


            glPushMatrix();

            float opengl_transform[16];
            biconvexTransform.localToWorld.store( opengl_transform );
            glMultMatrixf( opengl_transform );

            //RenderBiconvex( biconvex );
            GenerateBiconvexMesh( biconvex );       // HACK

            glPopMatrix();

            // render intersection between stone and mouse pick ray

            int mouse_x, mouse_y;
            GetMousePosition( mouse_x, mouse_y );

            vec3f rayStart, rayDirection;
            GetMousePickRay( mouse_x, mouse_y, rayStart, rayDirection );
            
            vec3f point,normal;
            float t = IntersectRayStone( biconvex, biconvexTransform, rayStart, rayDirection, point, normal );
            if ( t >= 0 )
            {
                glLineWidth( 2 );
                glColor4f( 0.7f,0,0,1 );
                glBegin( GL_LINES );
                vec3f p1 = point;
                vec3f p2 = point + normal * 0.1f;
                glVertex3f( p1.x(), p1.y(), p1.z() );
                glVertex3f( p2.x(), p2.y(), p2.z() );
                glEnd();
            }

            if ( mode == BiconvexSupport )
            {
                // render biconvex support along x axis

                vec3f biconvexCenter = biconvexTransform.GetPosition();
                vec3f biconvexUp = biconvexTransform.GetUp();
                float s1,s2;
                BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(1,0,0), s1, s2 );

                glLineWidth( 3 );
                glColor4f( 0,0,0.8f,1 );
                glBegin( GL_LINES );
                glVertex3f( s1, -10, 0 );
                glVertex3f( s1, +10, 0 );
                glVertex3f( s2, -10, 0 );
                glVertex3f( s2, +10, 0 );
                glEnd();
            }
        }
        else if ( mode == StoneAndBoard )
        {
            glMatrixMode( GL_PROJECTION );
            glLoadIdentity();
            const float fov = 25.0f;
            gluPerspective( fov, (float) displayWidth / (float) displayHeight, 0.1f, 100.0f );

            float flipX[] = { -1,0,0,0,
                               0,1,0,0,
                               0,0,1,0,
                               0,0,0,1 };

            glMultMatrixf( flipX );

            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();
            gluLookAt( 0, 5, -20.0f, 
                       0, 0, 0, 
                       0, 1, 0 );

            glEnable( GL_CULL_FACE );
            glCullFace( GL_BACK );
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

            // render board

            Board board( 20.0f, 20.0f, 1.0f );

            glLineWidth( 1 );
            glColor4f( 0.5f,0.5f,0.5f,1 );

            RenderBoard( board );

            // render stone

            const float targetRotation = rotating ? 0.28f : 0.0f;
            smoothedRotation += ( targetRotation - smoothedRotation ) * 0.15f;
            mat4f deltaRotation = mat4f::axisRotation( smoothedRotation, vec3f(1,2,3) );
            rotation = rotation * deltaRotation;

            RigidBodyTransform biconvexTransform( vec3f(0,biconvex.GetHeight()/2,0), rotation );

            glPushMatrix();

            float opengl_transform[16];
            biconvexTransform.localToWorld.store( opengl_transform );
            glMultMatrixf( opengl_transform );

            glEnable( GL_BLEND ); 
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glLineWidth( 1 );
            glColor4f( 0.5f,0.5f,0.5f,1 );
            RenderBiconvex( biconvex, 50, 10 );

            glPopMatrix();

            // render nearest intersection with picking ray: stone or board
            {
                int mouse_x, mouse_y;
                GetMousePosition( mouse_x, mouse_y );

                vec3f rayStart, rayDirection;
                GetMousePickRay( mouse_x, mouse_y, rayStart, rayDirection );

                vec3f board_point, board_normal, stone_point, stone_normal;
                const float board_t = IntersectRayBoard( board, rayStart, rayDirection, board_point, board_normal );
                const float stone_t = IntersectRayStone( biconvex, biconvexTransform, rayStart, rayDirection, stone_point, stone_normal );

                // display stone intersection if it is nearest or the board was not hit
                if ( ( stone_t >= 0.0f && board_t < 0.0f ) || ( stone_t >= 0.0f && stone_t < board_t ) )
                {
                    glLineWidth( 2 );
                    glColor4f( 0.7f,0,0,1 );
                    glBegin( GL_LINES );
                    vec3f p1 = stone_point;
                    vec3f p2 = stone_point + stone_normal * 0.5f;
                    glVertex3f( p1.x(), p1.y(), p1.z() );
                    glVertex3f( p2.x(), p2.y(), p2.z() );
                    glEnd();
                }

                // display board intersection if it is nearest, or we did not hit the stone
                if ( ( board_t >= 0.0f && stone_t < 0.0f ) || ( board_t >= 0.0f && board_t < stone_t ) )
                {
                    glLineWidth( 2 );
                    glColor4f( 0.7f,0,0,1 );
                    glBegin( GL_LINES );
                    vec3f p1 = board_point;
                    vec3f p2 = board_point + board_normal * 0.5f;
                    glVertex3f( p1.x(), p1.y(), p1.z() );
                    glVertex3f( p2.x(), p2.y(), p2.z() );
                    glEnd();

                    // if we hit the board, then render yellow line between board intersection
                    // and the nearest point on the stone. this we can test that nearest point
                    // on biconvex fn is working properly.
                    vec3f nearest = NearestPointOnStone( biconvex, biconvexTransform, board_point );
                    glLineWidth( 2 );
                    glColor4f( 0.9f,0.9f,0.3f,1 );
                    glBegin( GL_LINES );
                    glVertex3f( p1.x(), p1.y(), p1.z() );
                    glVertex3f( nearest.x(), nearest.y(), nearest.z() );
                    glEnd();
                }
            }

            // render intersection between stone and board
            {
                float depth;
                vec3f point,normal;
                if ( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) )
                {
                    glLineWidth( 2 );
                    glColor4f( 0,0,0.85f,1 );
                    glBegin( GL_LINES );
                    vec3f p1 = point;
                    vec3f p2 = point + normal * 0.5f;
                    glVertex3f( p1.x(), p1.y(), p1.z() );
                    glVertex3f( p2.x(), p2.y(), p2.z() );
                    glEnd();
                }
            }
        }
        else if ( mode == FallingStone || mode == LinearCollisionResponse || mode == AngularCollisionResponse || mode == CollisionResponseWithFriction )
        {
            const float fov = 25.0f;

            glMatrixMode( GL_PROJECTION );

            glLoadIdentity();
            
            float flipX[] = { -1,0,0,0,
                               0,1,0,0,
                               0,0,1,0,
                               0,0,0,1 };
            
            glMultMatrixf( flipX );

            gluPerspective( fov, (float) displayWidth / (float) displayHeight, 0.1f, 100.0f );

            glMatrixMode( GL_MODELVIEW );    

            glLoadIdentity();

            gluLookAt( 0, 5, -25.0f, 
                       0, 4, 0, 
                       0, 1, 0 );

            glEnable( GL_CULL_FACE );
            glCullFace( GL_BACK );
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

            // render board

            Board board( 20.0f, 20.0f, 1.0f );

            if ( mode != FallingStone )
            {
                glLineWidth( 1 );
                glColor4f( 0.5f,0.5f,0.5f,1 );
                RenderBoard( board );
            }

            // update stone physics

            // hack: substeps are required for the impulse
            // to look stable and not jitter at rest. why?!
            const int steps = ( mode == FallingStone ? 1 : 10 );

            const float step_dt = dt / steps * ( mode == FallingStone ? 0.1f : 1.0f );

            for ( int i = 0; i < steps; ++i )
            {
                RigidBodyTransform biconvexTransform( rigidBody.position, rigidBody.orientation );

                bool colliding = false;

                const float gravity = 9.8f * 10;    // cms/sec^2
                rigidBody.linearVelocity += vec3f(0,-gravity,0) * step_dt;

                if ( mode == LinearCollisionResponse )
                {
                    // if object is penetrating with the board, push it out

                    biconvexTransform = RigidBodyTransform( rigidBody.position, rigidBody.orientation );

                    float depth;
                    vec3f point, normal;
                    if ( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) )
                    {
                        rigidBody.position += normal * depth;
                        colliding = true;
                    }

                    // apply linear collision response

                    if ( colliding )
                    {
                        biconvexTransform = RigidBodyTransform( rigidBody.position, rigidBody.orientation );

                        vec3f stonePoint, stoneNormal, boardPoint, boardNormal;
                        ClosestFeaturesStoneBoard( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal );

                        const vec3f velocityAtPoint = rigidBody.linearVelocity;

                        const float e = 0.5f;

                        const float k = rigidBody.inverseMass;

                        const float j = max( - ( 1 + e ) * dot( velocityAtPoint, boardNormal ) / k, 0 );

                        rigidBody.linearVelocity += j * rigidBody.inverseMass * boardNormal;
                    }
                }
                else if ( mode == AngularCollisionResponse || mode == CollisionResponseWithFriction )
                {
                    // detect collision

                    biconvexTransform = RigidBodyTransform( rigidBody.position, rigidBody.orientation );

                    float depth;
                    vec3f point, normal;
                    if ( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) )
                    {
                        rigidBody.position += normal * depth;
                        colliding = true;
                    }

                    if ( colliding )
                    {
                        biconvexTransform = RigidBodyTransform( rigidBody.position, rigidBody.orientation );

                        vec3f stonePoint, stoneNormal, boardPoint, boardNormal;
                        ClosestFeaturesStoneBoard( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal );

                        vec3f p = stonePoint;
                        vec3f n = boardNormal;

                        vec3f r = p - rigidBody.position;

                        vec3f vp = rigidBody.linearVelocity + cross( rigidBody.angularVelocity, r );

                        const float vn = dot( vp, n );

                        if ( vn < 0 )
                        {
                            // calculate kinetic energy before collision response

                            const float ke_before = rigidBody.GetKineticEnergy();

                            // calculate inverse inertia tensor in world space

                            mat4f rotation;
                            rigidBody.orientation.toMatrix( rotation );
                            mat4f transposeRotation = transpose( rotation );
                            mat4f i = rotation * rigidBody.inverseInertiaTensor * transposeRotation;

                            // apply collision impulse

                            const float e = 0.6f;

                            const float linear_k = rigidBody.inverseMass;
                            const float angular_k = dot( cross( transformVector( i, cross( r, n ) ), r ), n );

                            const float k = linear_k + angular_k;

                            const float j = - ( 1 + e ) * vn / k;

                            rigidBody.linearVelocity += j * rigidBody.inverseMass * n;
                            rigidBody.angularVelocity += j * transformVector( i, cross( r, n ) );

                            // apply friction impulse

                            if ( mode == CollisionResponseWithFriction )
                            {
                                vec3f tangent_velocity = vp - n * dot( vp, n );

                                if ( length_squared( tangent_velocity ) > 0.0001f * 0.0001f )
                                {
                                    vec3f t = normalize( tangent_velocity );

                                    float u = 0.15f;

                                    const float vt = dot( vp, t );

                                    const float kt = rigidBody.inverseMass + dot( cross( transformVector( i, cross( r, t ) ), r ), t );

                                    const float jt = clamp( -vt / kt, -u * j, u * j );

                                    rigidBody.linearVelocity += jt * rigidBody.inverseMass * t;
                                    rigidBody.angularVelocity += jt * transformVector( i, cross( r, t ) );
                                }
                            }

                            // calculate kinetic energy after collision response
                            // IMPORTANT: verify that we never add energy!

                            const float ke_after = rigidBody.GetKineticEnergy();

                            assert( ke_after <= ke_before + 0.001f );
                        }
                    }
                }

                // integrate with velocities post collision response

                rigidBody.position += rigidBody.linearVelocity * step_dt;
                quat4f spin = AngularVelocityToSpin( rigidBody.orientation, rigidBody.angularVelocity );
                rigidBody.orientation += spin * step_dt;
                rigidBody.orientation = normalize( rigidBody.orientation );
            }

            // render stone

            RigidBodyTransform biconvexTransform = RigidBodyTransform( rigidBody.position, rigidBody.orientation );

            glPushMatrix();

            float opengl_transform[16];
            biconvexTransform.localToWorld.store( opengl_transform );
            glMultMatrixf( opengl_transform );

            glEnable( GL_BLEND ); 
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glLineWidth( 1 );
            glColor4f( 0.5f,0.5f,0.5f,1 );
            RenderBiconvex( biconvex, 50, 10 );

            glPopMatrix();
        }

        // update the display
        
        UpdateDisplay( 1 );

        // update time

        t += dt;
    }

    CloseDisplay();

    return 0;
}
