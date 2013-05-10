/*
  Vectorial
  Copyright (c) 2010 Mikko Lehtonen
  Licensed under the terms of the two-clause BSD License (see LICENSE)
*/
#ifndef VECTORIAL_MAT3F_H
#define VECTORIAL_MAT3F_H

#include "mat4f.h"

namespace vectorial 
{
    struct mat3f 
    {
        float data[9];
    
        inline mat3f() {}

        inline void load( mat4f & matrix ) 
        {
            data[0] = simd4f_get_x( matrix.value.x );
            data[1] = simd4f_get_y( matrix.value.x );
            data[2] = simd4f_get_z( matrix.value.x );

            data[3] = simd4f_get_x( matrix.value.y );
            data[4] = simd4f_get_y( matrix.value.y );
            data[5] = simd4f_get_z( matrix.value.y );

            data[6] = simd4f_get_x( matrix.value.z );
            data[7] = simd4f_get_y( matrix.value.z );
            data[8] = simd4f_get_z( matrix.value.z );
        }
    };
}

#endif
