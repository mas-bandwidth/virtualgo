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
    float depth;
    vec3f offset;
    vec3f impulse;
    vec3f intersectionPoint;
};

typedef std::map<TouchHandle,SelectData> SelectMap;

#endif