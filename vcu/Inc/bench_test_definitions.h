#ifndef VCU_BENCH_TEST_DEFINITIONS_H
#define VCU_BENCH_TEST_DEFINITIONS_H

/* Enter independently established expectations and limits once during initial
 * validation, record who confirmed them, and rebuild. These draft expectations
 * describe the existing conversion code; they are not validated 2027 hardware
 * measurements. Do not copy a DUT reading into its own expected value merely
 * to obtain a pass. vcuTest run requires validation below; limits are saved here.
 */
#define VCU_BENCH_DEFINITIONS_VALIDATED 0
#define VCU_BENCH_DEFINITIONS_REVISION "DRAFT-unvalidated"
#define VCU_BENCH_CONFIRMED_BY "UNCONFIRMED"
#define VCU_BENCH_PERCENT_TOLERANCE 0.0f
#define VCU_BENCH_ANGLE_TOLERANCE 0.0f

#define VCU_BENCH_THROTTLE_A_POINTS {0, 50, 100}
#define VCU_BENCH_THROTTLE_B_POINTS {0, 50, 100}
#define VCU_BENCH_THROTTLE_POINTS {0, 50, 100}
#define VCU_BENCH_BRAKE_POSITION_POINTS {0, 50, 100}
#define VCU_BENCH_BRAKE_PRESSURE_POINTS {0, 50, 100}
#define VCU_BENCH_BRAKE_COMBINED_POINTS {53, 80, 0}
#define VCU_BENCH_STEERING_POINTS {-102, 0, 102}

#endif
