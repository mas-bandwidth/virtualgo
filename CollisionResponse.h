#ifndef COLLISION_RESPONSE_H
#define COLLISION_RESPONSE_H

#include "RigidBody.h"
#include "CollisionDetection.h"

void ApplyLinearCollisionImpulse( StaticContact & contact, float e )
{
	const vec3f velocityAtPoint = contact.rigidBody->GetVelocityAtWorldPoint( contact.point );
    const float k = contact.rigidBody->inverseMass;
    const float j = max( - ( 1 + e ) * dot( velocityAtPoint, contact.normal ) / k, 0 );
    contact.rigidBody->linearMomentum += j * contact.normal;
}

void ApplyCollisionImpulseWithFriction( StaticContact & contact, float e, float u, float epsilon = 0.001f )
{
	RigidBody & rigidBody = *contact.rigidBody;

    vec3f velocityAtPoint = rigidBody.GetVelocityAtWorldPoint( contact.point );

    assert( velocityAtPoint.x() == velocityAtPoint.x() );
    assert( velocityAtPoint.y() == velocityAtPoint.y() );
    assert( velocityAtPoint.z() == velocityAtPoint.z() );
    
    const float vn = min( 0, dot( velocityAtPoint, contact.normal ) );

    const float ke_before_collision = rigidBody.GetKineticEnergy();

    // calculate inverse inertia tensor in world space

    mat4f rotation;
    rigidBody.orientation.toMatrix( rotation );
    mat4f transposeRotation = transpose( rotation );
    mat4f i = rotation * rigidBody.inverseInertiaTensor * transposeRotation;

    // apply collision impulse

    const vec3f r = contact.point - rigidBody.position;

    const float k = rigidBody.inverseMass + 
                    dot( cross( r, contact.normal ), 
                         transformVector( i, cross( r, contact.normal ) ) );

    const float j = - ( 1 + e ) * vn / k;

    assert( j == j );
    assert( k == k );

    assert( contact.normal.x() == contact.normal.x() );
    assert( contact.normal.y() == contact.normal.y() );
    assert( contact.normal.z() == contact.normal.z() );

    rigidBody.linearMomentum += j * contact.normal;
    rigidBody.angularMomentum += j * cross( r, contact.normal );

    assert( rigidBody.linearMomentum.x() == rigidBody.linearMomentum.x() );
    assert( rigidBody.linearMomentum.y() == rigidBody.linearMomentum.y() );
    assert( rigidBody.linearMomentum.z() == rigidBody.linearMomentum.z() );

    assert( rigidBody.angularMomentum.x() == rigidBody.angularMomentum.x() );
    assert( rigidBody.angularMomentum.y() == rigidBody.angularMomentum.y() );
    assert( rigidBody.angularMomentum.z() == rigidBody.angularMomentum.z() );

    const float ke_after_collision = rigidBody.GetKineticEnergy();

    assert( ke_after_collision <= ke_before_collision + epsilon );

    // apply friction impulse

    velocityAtPoint = rigidBody.GetVelocityAtWorldPoint( contact.point );

    vec3f tangentVelocity = velocityAtPoint - contact.normal * dot( velocityAtPoint, contact.normal );

    if ( length_squared( tangentVelocity ) > epsilon * epsilon )
    {
        vec3f tangent = normalize( tangentVelocity );

        const float vt = dot( velocityAtPoint, tangent );

        const float kt = rigidBody.inverseMass + 
                         dot( cross( r, tangent ), 
                              transformVector( i, cross( r, tangent ) ) );

        const float jt = clamp( -vt / kt, -u * j, u * j );

        assert( jt == jt );
        assert( kt == kt );
        
        rigidBody.linearMomentum += jt * tangent;
        rigidBody.angularMomentum += jt * cross( r, tangent );

        assert( rigidBody.linearMomentum.x() == rigidBody.linearMomentum.x() );
        assert( rigidBody.linearMomentum.y() == rigidBody.linearMomentum.y() );
        assert( rigidBody.linearMomentum.z() == rigidBody.linearMomentum.z() );

        assert( rigidBody.angularMomentum.x() == rigidBody.angularMomentum.x() );
        assert( rigidBody.angularMomentum.y() == rigidBody.angularMomentum.y() );
        assert( rigidBody.angularMomentum.z() == rigidBody.angularMomentum.z() );
    }

    const float ke_after_friction = rigidBody.GetKineticEnergy();

    assert( ke_after_friction <= ke_after_collision + epsilon );
}

#endif
