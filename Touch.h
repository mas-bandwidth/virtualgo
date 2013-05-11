#ifndef TOUCH_H
#define TOUCH_H

typedef void* TouchHandle;

struct Touch
{
	TouchHandle handle;
	vec3f point;
	double timestamp;
};

#endif