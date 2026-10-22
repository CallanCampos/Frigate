#ifndef POKEBW2_DWC_GBUCKET_H
#define POKEBW2_DWC_GBUCKET_H

#include "types.h"
#include "dwc/hashtable.h"

// The buckets of GameSpy's stats (gbucket.c), in overlay 11: named values, an integer, a double or a text, that a game
// keeps in a hash table and changes by name, which gstats.c's operations of a game, its teams and its players go
// through

typedef enum BucketType {
    bt_int,
    bt_float,
    bt_string,
} BucketType;

// A set of buckets
typedef struct bucketset_s *bucketset_t;

struct bucketset_s {
    HashTable buckets;
};

// One value: its name, its type, how many values its average was made of, and the value
typedef struct bucket_s {
    char *name;
    BucketType type;
    int nvals;
    union {
        int ival;
        double fval;
        char *sval;
    } vals;
} bucket_t;

// The set that the functions use when they are given none
extern bucketset_t g_buckets;

// Each returns the bucket's new value (an int *, a double * or the text), or NULL if the set has no bucket of the name
void *BucketNew(bucketset_t set, char *name, BucketType type, void *initialvalue);
void *BucketSet(bucketset_t set, char *name, void *value);
// The operations change an int or a double by the value; on a text BucketAdd appends the value and the others do
// nothing
void *BucketAdd(bucketset_t set, char *name, void *value);
void *BucketSub(bucketset_t set, char *name, void *value);
void *BucketMult(bucketset_t set, char *name, void *value);
void *BucketDiv(bucketset_t set, char *name, void *value);
void *BucketConcat(bucketset_t set, char *name, void *value);
// Averages the value in, counting the values the bucket has had since it was set
void *BucketAvg(bucketset_t set, char *name, void *value);
// The address of a copy of an int or a double, for the functions above
void *bint(int i);
void *bfloat(double f);

#endif // POKEBW2_DWC_GBUCKET_H
