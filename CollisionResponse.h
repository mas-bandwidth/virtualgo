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

void ApplyCollisionImpulseWithFriction( StaticContact & contact, float e, float u )
{
	RigidBody & rigidBody = *contact.rigidBody;

    vec3f velocityAtPoint = rigidBody.GetVelocityAtWorldPoint( contact.point );

    const float vn = dot( velocityAtPoint, contact.normal );

    if ( vn < 0 )
    {
        // calculate kinetic energy before collision response

        const float ke_before_collision = rigidBody.GetKineticEnergy();

        // calculate inverse inertia tensor in world space

        mat4f rotation;
        rigidBody.orientation.toMatrix( rotation );
        mat4f transposeRotation = transpose( rotation );
        mat4f i = rotation * rigidBody.inverseInertiaTensor * transposeRotation;

        // apply collision impulse

	    const vec3f r = contact.point - rigidBody.position;

        const float k = rigidBody.inverseMass + dot( cross( r, contact.normal ), transformVector( i, cross( r, contact.normal ) ) );

        const float j = - ( 1 + e ) * vn / k;

        rigidBody.linearMomentum += j * contact.normal;
        rigidBody.angularMomentum += j * cross( r, contact.normal );

        const float ke_after_collision = rigidBody.GetKineticEnergy();
        assert( ke_after_collision <= ke_before_collision + 0.001f );

        // apply friction impulse

        velocityAtPoint = rigidBody.GetVelocityAtWorldPoint( contact.point );

        vec3f tangentVelocity = velocityAtPoint - contact.normal * dot( velocityAtPoint, contact.normal );

        if ( length_squared( tangentVelocity ) > 0.001f * 0.001f )
        {
            vec3f tangent = normalize( tangentVelocity );

            const float vt = dot( velocityAtPoint, tangent );

            const float kt = rigidBody.inverseMass + dot( cross( r, tangent ), transformVector( i, cross( r, tangent ) ) );

            const float jt = clamp( -vt / kt, -u * j, u * j );

            rigidBody.linearMomentum += jt * tangent;
            rigidBody.angularMomentum += jt * cross( r, tangent );

            const float ke_after_friction = rigidBody.GetKineticEnergy();
            assert( ke_after_friction <= ke_after_collision + 0.001f );
        }
    }
}

#endif
