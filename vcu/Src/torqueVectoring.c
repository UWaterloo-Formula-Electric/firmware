#include "torqueVectoring.h"

/**
 * The below algorithm assumes that:
 *      - Positive angle => clockwise rotation (right turn)
 *      - Negative angle => counterclockwise rotation (left turn)
 *
 * Also assumes that 0 degrees => no turn of steering wheel
 *
 * NEEDS TO BE TESTED!!
 */

// Currently does not account if value is negative - will wheel reverse in that case?

float motorRightTorque(float torqueRight, float steeringAngle)
{
    return torqueRight + steeringAngle * TV_FACTOR;
}

float motorLeftTorque(float torqueLeft, float steeringAngle)
{
    return torqueLeft - steeringAngle * TV_FACTOR;
}
