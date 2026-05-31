#pragma once

using namespace std;

#include "commons.h"

typedef pair<double, int> HeapItem;

typedef
    priority_queue<
        HeapItem, 
        vector<HeapItem>, 
        greater<HeapItem>
    >
    TimeHeap;


