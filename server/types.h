#ifndef __TYPES_H__
#define __TYPES_H__

typedef unsigned long millis_t;

typedef struct
{
    int8_t x_index, y_index;
    float distance; // When populated, the distance from the search location
} mesh_index_pair;

#endif
