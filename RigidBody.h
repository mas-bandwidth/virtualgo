#ifndef RIGID_BODY_H
#define RIGID_BODY_H

/*
    Rigid body class and support functions.
    We need a nice way to cache the local -> world,
    world -> local and position for a given rigid body.

    The rigid body transform class lets us do this,
    it's fundamentally a 4x4 rigid body transform matrix
    but with the inverse cached, as well as position, rotation
    and rotation inverse for quick lookup.
*/

inline void AngularVelocityToSpin( const quat4f & orientation, vec3f angularVelocity, quat4f & spin )
{
    spin = 0.5f * quat4f( 0, angularVelocity.x(), angularVelocity.y(), angularVelocity.z() ) * orientation;
}

struct RigidBodyTransform
{
    mat4f localToWorld;
    mat4f worldToLocal;
    
    void Initialize( const vec3f & position, const mat4f & rotation = mat4f::identity() )
    {
        localToWorld = rotation;
        localToWorld.value.w = simd4f_create( position.x(), position.y(), position.z(), 1 );
        RigidBodyInverse( localToWorld, worldToLocal );
    }
    
    void Initialize( const vec3f & position, const quat4f & rotation )
    {
        rotation.toMatrix( localToWorld );
        localToWorld.value.w = simd4f_create( position.x(), position.y(), position.z(), 1 );
        RigidBodyInverse( localToWorld, worldToLocal );
    }
    
    vec3f GetUp() const
    {
        return transformVector( localToWorld, vec3f(0,0,1) );
    }
    
    vec3f GetPosition() const
    {
        return localToWorld.value.w;
    }
};

struct RigidBody
{
    mat4f inertiaTensor;
    mat4f inverseInertiaTensor;
    mat4f inertiaTensorWorld;
    mat4f inverseInertiaTensorWorld;

    mat4f rotation, transposeRotation;              // IMPORTANT: these are secondary quantities calculated from orientation

    quat4f orientation;

    vec3f inertia;
    vec3f position;
    vec3f linearMomentum, angularMomentum;
    vec3f linearVelocity, angularVelocity;          // IMPORTANT: these are secondary quantities calculated from momentum

    float mass;
    float inverseMass;
    float deactivateTimer;

    bool active;

    RigidBody()
    {
        active = true;
        deactivateTimer = 0.0f;
        position = vec3f(0,0,0);
        orientation = quat4f::identity();
        linearMomentum = vec3f(0,0,0);
        angularMomentum = vec3f(0,0,0);
        mass = 1.0f;
        inverseMass = 1.0f / mass;
        inertia = vec3f(1,1,1);
        inertiaTensor = mat4f::identity();
        inverseInertiaTensor = mat4f::identity();

        UpdateOrientation();
        UpdateMomentum();
    }

    void UpdateOrientation()
    {
        orientation.toMatrix( rotation );
        transposeRotation = transpose( rotation );
        inertiaTensorWorld = rotation * inertiaTensor * transposeRotation;
        inverseInertiaTensorWorld = rotation * inverseInertiaTensor * transposeRotation;
    }

    void GetTransform( RigidBodyTransform & transform ) const
    {
        transform.Initialize( position, rotation );         // todo: can do it faster. we already have the transpose
    }

    void UpdateMomentum()
    {
        if ( active )
        {
            const float MaxAngularMomentum = 10;

            float x = clamp( angularMomentum.x(), -MaxAngularMomentum, MaxAngularMomentum );
            float y = clamp( angularMomentum.y(), -MaxAngularMomentum, MaxAngularMomentum );
            float z = clamp( angularMomentum.z(), -MaxAngularMomentum, MaxAngularMomentum );

            angularMomentum = vec3f( x,y,z );

            linearVelocity = linearMomentum * inverseMass;
            angularVelocity = transformVector( inverseInertiaTensorWorld, angularMomentum );
        }
        else
        {
            linearMomentum = vec3f( 0,0,0 );
            linearVelocity = vec3f( 0,0,0 );
            angularMomentum = vec3f( 0,0,0 );
            angularVelocity = vec3f( 0,0,0 );
        }
    }

    void GetVelocityAtWorldPoint( vec3f point, vec3f & velocity ) const
    {
        vec3f angularVelocity = transformVector( inverseInertiaTensorWorld, angularMomentum );
        velocity = linearVelocity + cross( angularVelocity, point - position );
    }

    float GetKineticEnergy() const
    {
        // IMPORTANT: please refer to http://people.rit.edu/vwlsps/IntermediateMechanics2/Ch9v5.pdf
        // for the derivation of angular kinetic energy from angular velocity + inertia tensor

        const float linearKE = length_squared( linearMomentum ) / ( 2 * mass );

        vec3f angularMomentumLocal = transformVector( transposeRotation, angularMomentum );
        vec3f angularVelocityLocal = transformVector( inverseInertiaTensor, angularMomentumLocal );

        const float ix = inertia.x();
        const float iy = inertia.y();
        const float iz = inertia.z();

        const float wx = angularVelocityLocal.x();
        const float wy = angularVelocityLocal.y();
        const float wz = angularVelocityLocal.z();

        const float angularKE = 0.5f * ( ix * wx * wx + 
                                         iy * wy * wy +
                                         iz * wz * wz );

        return linearKE + angularKE;
    }

    void Activate()
    {
        if ( !active )
            active = true;
    }

    void Deactivate()
    {
        if ( active )
        {
            active = false;
            deactivateTimer = 0;
            linearMomentum = vec3f(0,0,0);
            linearVelocity = vec3f(0,0,0);
            angularMomentum = vec3f(0,0,0);
            angularVelocity = vec3f(0,0,0);
        }
    }

    void ApplyImpulse( const vec3f & impulse )
    {
        Activate();
        linearMomentum += impulse;
        UpdateMomentum();                   // todo: can this be avoided?
    }

    void ApplyImpulseAtWorldPoint( const vec3f & point, const vec3f & impulse )
    {
        Activate();
        vec3f r = point - position;
        linearMomentum += impulse;
        angularMomentum += cross( r, impulse );
        UpdateMomentum();                   // todo: can this be avoided?
    }
};

#endif
