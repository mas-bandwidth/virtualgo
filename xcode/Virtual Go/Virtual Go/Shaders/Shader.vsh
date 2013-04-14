//
//  Shader.vsh
//  Virtual Go
//
//  Created by Glenn Fiedler on 4/13/13.
//  Copyright (c) 2013 Glenn Fiedler. All rights reserved.
//

attribute vec4 position;
attribute vec3 normal;

varying lowp vec4 colorVarying;

uniform mat4 modelViewProjectionMatrix;
uniform mat3 normalMatrix;

void main()
{
    vec3 eyeNormal = normalize( normalMatrix * normal );
    vec3 lightPosition = vec3( 0.0, 1.0, 0.0 );
    vec4 diffuseColor = vec4( 1.0, 1.0, 1.0, 1.0 );
    vec4 ambientColor = vec4( 0.5, 0.5, 0.5, 1.0 );
    
    float nDotVP = max( 0.0, dot( eyeNormal, normalize( lightPosition ) ) );
                 
    colorVarying = ambientColor + diffuseColor * nDotVP;
    
    gl_Position = modelViewProjectionMatrix * position;
}
