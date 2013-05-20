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
    uint32_t stoneId;
    TouchHandle touchHandle;
    vec3f point;
    float depth;
    vec3f offset;
    double timestamp;
    vec3f intersectionPoint;
    vec3f prevIntersectionPoint;
    vec3f touchImpulse;
    bool moved;
};

typedef std::map<TouchHandle,SelectData> SelectMap;

#endif