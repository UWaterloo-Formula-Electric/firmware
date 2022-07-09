#ifndef TORQUEVECTORING_H
#define TORQUEVECTORING_H

#define TV_FACTOR 2.5   // Can be adjusted when testing

float motorRightTorque(float torqueRight, float steeringAngle);
float motorLeftTorque(float torqueLeft, float steeringAngle);

#endif