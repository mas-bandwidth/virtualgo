#ifndef TOUCH_H
#define TOUCH_H

#include <map>

typedef void* TouchHandle;

struct Touch
{
	TouchHandle handle;
	vec3f point;
	double timestamp;
};

struct SelectData
{
    Touch touch;
    uint32_t stoneId : 16;
    uint32_t moved : 1;
    uint32_t constrained : 1;
    uint32_t constraintRow : 8;
    uint32_t constraintColumn : 8;
    float depth;
    vec3f offset;
    vec3f impulse;
    vec3f intersectionPoint;
    vec3f lastMoveDelta;
    vec3f originalPosition;
};

typedef std::map<TouchHandle,SelectData> SelectMap;

#endif