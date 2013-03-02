#ifndef INERTIA_TENSOR_H
#define INERTIA_TENSOR_H

void CalculateSphereInertiaTensor( float mass, float r, mat4f & inertiaTensor, mat4f & inverseInertiaTensor )
{
    const float i = 2.0f / 5.0f * mass * r * r;
    float values[] = { i, 0, 0, 0, 
                       0, i, 0, 0,
                       0, 0, i, 0,
                       0, 0, 0, 1 };
    float inverse_values[] = { 1/i,  0,   0,   0, 
                                0, 1/i,   0,   0, 
                                0,   0, 1/i,   0,
                                0,   0,   0,   1 };
    inertiaTensor.load( values );
    inverseInertiaTensor.load( inverse_values );
}

void CalculateEllipsoidInertiaTensor( float mass, float a, float b, float c, mat4f & inertiaTensor, mat4f & inverseInertiaTensor )
{
    const float i_a = 1.0f/5.0f * mass * ( b*b + c*c );
    const float i_b = 1.0f/5.0f * mass * ( a*a + c*c );
    const float i_c = 1.0f/5.0f * mass * ( a*a + b*b );
    float values[] = { i_a,   0,   0, 0, 
                         0, i_b,   0, 0,
                         0,   0, i_c, 0,
                         0,   0,   0, 1 };
    float inverse_values[] = { 1/i_a,     0,     0,   0, 
                                   0, 1/i_b,     0,   0, 
                                   0,     0, 1/i_c,   0,
                                   0,     0,     0,   1 };
    inertiaTensor.load( values );
    inverseInertiaTensor.load( inverse_values );
}

float CalculateBiconvexVolume( const Biconvex & biconvex )
{
    const float r = biconvex.GetSphereRadius();
    const float h = r - biconvex.GetHeight() / 2;
    return h*h + ( pi * r / 4 + pi * h / 24 );
}

void CalculateBiconvexInertiaTensor( float mass, const Biconvex & biconvex, vec3f & inertia, mat4f & inertiaTensor, mat4f & inverseInertiaTensor )
{
    const double resolution = 0.01;
    const double width = biconvex.GetWidth();
    const double height = biconvex.GetHeight();
    const double xz_steps = ceil( width / resolution );
    const double y_steps = ceil( height / resolution );
    const double dx = width / xz_steps;
    const double dy = height / y_steps;
    const double dz = width / xz_steps;
    double sx = -width / 2;
    double sy = -height / 2;
    double sz = -width / 2;
    double ix = 0.0;
    double iy = 0.0;
    double iz = 0.0;
    const double v = CalculateBiconvexVolume( biconvex );
    const double p = mass / v;
    const double m = dx*dy*dz * p;
    for ( int index_z = 0; index_z <= xz_steps; ++index_z )
    {
        for ( int index_y = 0; index_y <= y_steps; ++index_y )
        {
            for ( int index_x = 0; index_x <= xz_steps; ++index_x )
            {
                const double x = sx + index_x * dx;
                const double y = sy + index_y * dy;
                const double z = sz + index_z * dz;

                vec3f point(x,y,z);

                if ( PointInsideBiconvex_LocalSpace( point, biconvex ) )
                {
                    const double rx2 = z*z + y*y;
                    const double ry2 = x*x + z*z;
                    const double rz2 = x*x + y*y;

                    ix += rx2 * m;
                    iy += ry2 * m;
                    iz += rz2 * m;
                }
            }
        }
    }

    {
        // http://wolframalpha.com
        // integrate ( r^2 - ( y + r - h/2 ) ^ 2 ) ^ 2 dy from y = 0 to h/2
        //  => 1/480 h^3 (3 h^2-30 h r+80 r^2)
        const float h = height;
        const float r = biconvex.GetSphereRadius();
        const float h2 = h * h;
        const float h3 = h2 * h;
        const float h4 = h3 * h;
        const float h5 = h4 * h;
        const float r2 = r * r;
        const float r3 = r2 * r;
        const float r4 = r3 * r;
        const float exact_iy = pi * p * ( 1/480.0f * h3 * ( 3*h2 - 30*h*r + 80*r2 ) );
    }

    const float inertiaValues[] = { ix, iy, iz };

    const float inertiaTensorValues[] = { ix, 0,  0, 0, 
                                          0, iy,  0, 0, 
                                          0,  0, iz, 0, 
                                          0,  0,  0, 1 };

    const float inverseInertiaTensorValues[] = { 1/ix,    0,    0, 0, 
                                                    0, 1/iy,    0, 0, 
                                                    0,    0, 1/iz, 0, 
                                                    0,    0,    0, 1 };

    inertia.load( inertiaValues );
    inertiaTensor.load( inertiaTensorValues );
    inverseInertiaTensor.load( inverseInertiaTensorValues );
}

#endif
