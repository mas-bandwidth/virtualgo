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

struct RigidBody
{
    vec3f position;
    quat4f orientation;
    vec3f linearMomentum, angularMomentum;
    vec3f linearVelocity, angularVelocity;          // IMPORTANT: these are secondary quantities calculated from momentum
    float mass;
    float inverseMass;
    vec3f inertia;
    mat4f inertiaTensor;
    mat4f inverseInertiaTensor;

    RigidBody()
    {
        position = vec3f(0,0,0);
        orientation = quat4f::identity();
        linearMomentum = vec3f(0,0,0);
        angularMomentum = vec3f(0,0,0);
        linearVelocity = vec3f(0,0,0);
        angularVelocity = vec3f(0,0,0);
        mass = 1.0f;
        inverseMass = 1.0f / mass;
        inertia = vec3f(1,1,1);
        inertiaTensor = mat4f::identity();
        inverseInertiaTensor = mat4f::identity();
    }

    void Update()
    {
        const float MaxAngularMomentum = 10;
        float x = clamp( angularMomentum.x(), -MaxAngularMomentum, MaxAngularMomentum );
        float y = clamp( angularMomentum.y(), -MaxAngularMomentum, MaxAngularMomentum );
        float z = clamp( angularMomentum.z(), -MaxAngularMomentum, MaxAngularMomentum );
        angularMomentum = vec3f( x,y,z );

        linearVelocity = linearMomentum * inverseMass;
        angularVelocity = transformVector( inverseInertiaTensor, angularMomentum );
    }

    vec3f GetVelocityAtWorldPoint( vec3f point ) const
    {
        // IMPORTANT: angular momentum may have been updated without updating angular velocity
        vec3f angularVelocity = transformVector( inverseInertiaTensor, angularMomentum );
        return linearVelocity + cross( angularVelocity, point - position );
    }

    float GetKineticEnergy() const
    {
        // IMPORTANT: please refer to http://people.rit.edu/vwlsps/IntermediateMechanics2/Ch9v5.pdf
        // for the derivation of angular kinetic energy from angular velocity + inertia tensor

        const float linearKE = length_squared( linearMomentum ) / ( 2 * mass );

        vec3f angularVelocity = transformVector( inverseInertiaTensor, angularMomentum );

        const float ix = inertia.x();
        const float iy = inertia.y();
        const float iz = inertia.z();

        const float wx = angularVelocity.x();
        const float wy = angularVelocity.y();
        const float wz = angularVelocity.z();

        const float angularKE = 0.5f * ( ix * wx * wx + 
                                         iy * wy * wy +
                                         iz * wz * wz );

        return linearKE + angularKE;
    }
};

inline quat4f AngularVelocityToSpin( const quat4f & orientation, vec3f angularVelocity )
{
    return 0.5f * quat4f( 0, angularVelocity.x(), angularVelocity.y(), angularVelocity.z() ) * orientation;
}

struct RigidBodyTransform
{
    mat4f localToWorld;
    mat4f worldToLocal;

    RigidBodyTransform( vec3f position, mat4f rotation = mat4f::identity() )
    {
        localToWorld = rotation;
        localToWorld.value.w = simd4f_create( position.x(), position.y(), position.z(), 1 );
        worldToLocal = RigidBodyInverse( localToWorld );
    }

    RigidBodyTransform( vec3f position, quat4f rotation )
    {
        rotation.toMatrix( localToWorld );
        localToWorld.value.w = simd4f_create( position.x(), position.y(), position.z(), 1 );
        worldToLocal = RigidBodyInverse( localToWorld );
    }

    vec3f GetUp() const
    {
        // get on up
        return transformVector( localToWorld, vec3f(0,1,0) );
    }

    vec3f GetPosition() const
    {
        // stay on the scene
        return localToWorld.value.w;
    }
};

#endif
