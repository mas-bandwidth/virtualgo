#ifndef CONFIG_H
#define CONFIG_H

#if defined( __APPLE__ ) && TARGET_OS_IPHONE
#define OPENGL_ES2 1
#endif

#define STONES 1
#define SHADOWS 1
#define PHYSICS 1
#define VALIDATION 0

const int RenderToTextureSize = 1024;

const int StoneTessellationLevel = 3;
const int StoneShadowTessellationLevel = 2;

const float DropMomentum = 10;

const float DeleteTime = 2;

const float PlacementVariance = 0.2f;
const float ConstraintDelta = 0.5f;

const int MaxStones = 128;

const float SceneGridRes = 4;
const float SceneGridWidth = 64;
const float SceneGridHeight = 64;
const float SceneGridDepth = 64;

const float FatFingerBonus = 1.35f;

const int BoardSize = 9;

const int MaxStarPoints = 9;

const int MaxTouches = 64;

const float AccelerometerFrequency = 20;
const float AccelerometerTightness = 0.1f;

const float JerkThreshold = 0.1f;
const float JerkScale = 1.5f;
const float JerkMax = 0.75f;
const float JerkVariance = 0.25f;

const float LaunchThreshold = 0.625f;
const float LaunchMomentum = 8;
const float LaunchVariance = 0.1f;

const float SelectDamping = 0.75f;
const float SelectHeight = 3.0f;

// while in a game where the stones cannot be moved
// these are good settings.
/*
const float TouchImpulse = 5.0f;
const float SelectImpulse = 1.0f;
*/

// these settings work best for edit mode when stones
// are selected and lifted up.
const float TouchImpulse = 2.5f;
const float SelectImpulse = 0.25f;

#endif
