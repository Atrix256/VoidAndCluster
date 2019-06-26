
#pragma once

#define DETERMINISTIC() true  // if true, will use the seed below for everything, else will randomly generate a seed.

#define DETERMINISTIC_SEED() unsigned(783104853), unsigned(4213684301), unsigned(3526061164), unsigned(614346169), unsigned(478811579), unsigned(2044310268), unsigned(3671768129), unsigned(206439072)

#define THRESHOLD_SAMPLES() 11 // the number of samples for threshold testing.

#define SAVE_VOIDCLUSTER_INITIALBP() false
#define SAVE_VOIDCLUSTER_PHASE1() false