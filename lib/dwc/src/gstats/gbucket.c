#include "dwc/gbucket.h"
#include "dwc/ext.h"

// The buckets of GameSpy's stats (gbucket.c). The ROM names neither the file nor its functions: the file, BucketNew,
// BucketSet, BucketAdd, BucketSub, BucketMult, BucketDiv, BucketConcat and BucketAvg are GameSpy's, as pret's
// pokerevo Wii build has them (gbucket.s), and bint, bfloat, stripchars, DoSet, DoGet, DoFind and g_buckets are the
// later SDK's

static char *stripchars(char *s);
static void *DoSet(GBucket *bucket, void *value);
static void *DoGet(GBucket *bucket);
static GBucket *DoFind(GBucketSet *set, char *name);

GBucketSet *g_buckets = NULL;

void *BucketNew(GBucketSet *set, char *name, BucketType type, void *initialvalue) {
    GBucket bucket;

    if (set == NULL) {
        set = g_buckets;
    }
    bucket.name = goastrdup(name);
    bucket.type = type;
    bucket.vals.sval = NULL;
    bucket.nvals = 1;
    DoSet(&bucket, initialvalue);
    func_ov011_021718dc(set->buckets, &bucket);
    return DoGet(DoFind(set, name));
}

void *BucketSet(GBucketSet *set, char *name, void *value) {
    GBucket *bucket = DoFind(set, name);

    if (bucket == NULL) {
        return NULL;
    }
    bucket->nvals = 0;
    return DoSet(bucket, value);
}

void *BucketAdd(GBucketSet *set, char *name, void *value) {
    GBucket *bucket = DoFind(set, name);

    if (bucket == NULL) {
        return NULL;
    }
    if (bucket->type == bt_int) {
        return DoSet(bucket, bint(*(int *)DoGet(bucket) + *(int *)value));
    }
    if (bucket->type == bt_float) {
        return DoSet(bucket, bfloat(*(double *)DoGet(bucket) + *(double *)value));
    }
    return BucketConcat(set, name, value);
}

void *BucketSub(GBucketSet *set, char *name, void *value) {
    GBucket *bucket = DoFind(set, name);

    if (bucket == NULL) {
        return NULL;
    }
    if (bucket->type == bt_int) {
        return DoSet(bucket, bint(*(int *)DoGet(bucket) - *(int *)value));
    }
    if (bucket->type == bt_float) {
        return DoSet(bucket, bfloat(*(double *)DoGet(bucket) - *(double *)value));
    }
    return DoGet(bucket);
}

void *BucketMult(GBucketSet *set, char *name, void *value) {
    GBucket *bucket = DoFind(set, name);

    if (bucket == NULL) {
        return NULL;
    }
    if (bucket->type == bt_int) {
        return DoSet(bucket, bint(*(int *)DoGet(bucket) * *(int *)value));
    }
    if (bucket->type == bt_float) {
        return DoSet(bucket, bfloat(*(double *)DoGet(bucket) * *(double *)value));
    }
    return DoGet(bucket);
}

void *BucketDiv(GBucketSet *set, char *name, void *value) {
    GBucket *bucket = DoFind(set, name);

    if (bucket == NULL) {
        return NULL;
    }
    if (bucket->type == bt_int) {
        return DoSet(bucket, bint(*(int *)DoGet(bucket) / *(int *)value));
    }
    if (bucket->type == bt_float) {
        return DoSet(bucket, bfloat(*(double *)DoGet(bucket) / *(double *)value));
    }
    return DoGet(bucket);
}

void *BucketConcat(GBucketSet *set, char *name, void *value) {
    GBucket *bucket = DoFind(set, name);
    char *temp;
    char *s;

    if (bucket == NULL) {
        return NULL;
    }
    s = DoGet(bucket);
    temp = gsimalloc(strlen(s) + strlen(value) + 1);
    strcpy(temp, s);
    strcat(temp, value);
    DoSet(bucket, temp);
    gsifree(temp);
    return DoGet(bucket);
}

// The average of nvals values and one more, which counts it
#define BUCKET_AVG(cur, new, num) (((cur) * (num) + (new)) / (++(num)))

void *BucketAvg(GBucketSet *set, char *name, void *value) {
    GBucket *bucket = DoFind(set, name);

    if (bucket == NULL) {
        return NULL;
    }
    if (bucket->type == bt_int) {
        return DoSet(bucket, bint(BUCKET_AVG(*(int *)DoGet(bucket), *(int *)value, bucket->nvals)));
    }
    if (bucket->type == bt_float) {
        return DoSet(bucket, bfloat(BUCKET_AVG(*(double *)DoGet(bucket), *(double *)value, bucket->nvals)));
    }
    return DoGet(bucket);
}

void *bint(int i) {
    static int j;

    j = i;
    return &j;
}

void *bfloat(double f) {
    static double g;

    g = f;
    return &g;
}

// Turns the backslashes of a text, which separate the keys and values of the stats' messages, into slashes
static char *stripchars(char *s) {
    char *p = s;

    while (*s != '\0') {
        if (*s == '\\') {
            *s = '/';
        }
        s++;
    }
    return p;
}

static void *DoSet(GBucket *bucket, void *value) {
    if (bucket->type == bt_int) {
        bucket->vals.ival = *(int *)value;
    } else if (bucket->type == bt_float) {
        bucket->vals.fval = *(double *)value;
    } else if (bucket->type == bt_string) {
        if (bucket->vals.sval != NULL) {
            gsifree(bucket->vals.sval);
        }
        bucket->vals.sval = value == NULL ? NULL : stripchars(goastrdup(value));
    }
    return DoGet(bucket);
}

static void *DoGet(GBucket *bucket) {
    if (bucket == NULL) {
        return NULL;
    }
    if (bucket->type == bt_string) {
        return bucket->vals.sval;
    }
    return &bucket->vals;
}

static GBucket *DoFind(GBucketSet *set, char *name) {
    GBucket key;

    if (set == NULL) {
        set = g_buckets;
    }
    key.name = name;
    return func_ov011_02171964(set->buckets, &key);
}
