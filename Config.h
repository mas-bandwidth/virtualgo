#ifndef CONFIG_H
#define CONFIG_H

#if defined( __APPLE__ ) && TARGET_OS_IPHONE
#define OPENGL_ES2 1
#endif

const float DeleteTime = 2;

const float PlacementVariance = 0.2f;
const float ConstraintDelta = 0.5f;

const int MaxStones = 256;

const float SceneGridRes = 4;
const float SceneGridWidth = 64;
const float SceneGridHeight = 64;
const float SceneGridDepth = 64;

const float FatFingerBonus = 1.35f;

const int BoardSize = 9;

const int MaxStarPoints = 9;

const int MaxTouches = 64;

const float PlaceStoneHardThreshold = 0.075f;

const float ZoomIn = 25;
const float ZoomOut = 48;

const float ZoomInTightness = 0.25f;
const float ZoomOutTightness = 0.15f;

const float AccelerometerFrequency = 20;
const float AccelerometerTightness = 0.1f;

const float JerkThreshold = 0.1f;
const float JerkScale = 1.5f;
const float JerkMax = 0.75f;
const float JerkVariance = 0.25f;

const float LaunchThreshold = 0.625f;
const float LaunchMomentum = 8;
const float LaunchVariance = 0.1f;

const float MinimumSwipeLength = 50;            // points
const float SwipeLengthPerSecond = 250;         // points
const float MaxSwipeTime = 1.0f;                // seconds
const float SwipeMomentum = 10.0f;

const float HoldDelay = 0.05f;                  // seconds
const float HoldDamping = 0.75f;                
const float HoldMoveThreshold = 40;             // points

const float SelectDamping = 0.75f;

const float TouchImpulse = 5.0f;

#endif
