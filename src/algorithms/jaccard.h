#ifndef JACCARD_H
#define JACCARD_H

#include "../core/minhash.h"

float jaccard_similarity(const MinHash* mh1, const MinHash* mh2);

#endif // JACCARD_H
