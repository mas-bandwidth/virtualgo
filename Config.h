#ifndef CONFIG_H
#define CONFIG_H

#if defined( __APPLE__ ) && TARGET_OS_IPHONE
#define OPENGL_ES2 1
#endif

#define STONE_DEMO 1
#define STONES 1
#define SHADOWS 1
#define PHYSICS 1
#define VALIDATION 0

const float Ceiling = 50;

const int RenderToTextureSize = 2048;

const int StoneTessellationLevel = 5;
const int StoneShadowTessellationLevel = 3;

const float DropMomentum = 10;

const float DeleteTime = 2;

const float PlacementVariance = 0.2f;
const float ConstraintDelta = 0.5f;

#if STONE_DEMO
const int MaxStones = 128;
#else
const int MaxStones = 1;
#endif

const float SceneGridRes = 4;
const float SceneGridWidth = 64;
const float SceneGridHeight = 64;
const float SceneGridDepth = 64;

const float FatFingerBonus = 1.35f;

const int BoardSize = 9;

const int MaxStarPoints = 9;

#if STONE_DEMO
const int MaxTouches = 1;
#else
const int MaxTouches = 64;
#endif

const float AccelerometerFrequency = 30;
const float AccelerometerTightness = 0.1f;

const float JerkThreshold = 0.1f;
const float JerkScale = 1.0f;
const float JerkMax = 0.1f;
const float JerkVariance = 0.025f;

const float LaunchThreshold = 0.9;
const float LaunchMomentumScale = 4.0;
const float LaunchVariance = 0.1f;

#if STONE_DEMO
const float SelectDamping = 0.95f;
#else
const float SelectDamping = 0.85f;
#endif
const float SelectHeight = 3.0f;

#if STONE_DEMO
const float TouchImpulse = 3.0f;
const float SelectImpulse = 3.0f;
#else
const float TouchImpulse = 2.5f;
const float SelectImpulse = 0.25f;
#endif

const float MinimumSwipeLength = 50;            // points
const float SwipeLengthPerSecond = 150;         // points
const float MaxSwipeTime = 1.0f;                // seconds
const float SwipeMomentum = 10.0f;

#endif