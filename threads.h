// threads.h

#ifndef THREADS_H
#define THREADS_H

// runs an agent loop that places two ingredients and wakes pushers
void* agentThread(void* arg);

// handles tobacco arrivals and wakes the matching smoker when a pair is complete
void* tobaccoPusher(void* arg);

// handles paper arrivals and wakes the matching smoker when a pair is complete
void* paperPusher(void* arg);

// handles match arrivals and wakes the matching smoker when a pair is complete
void* matchPusher(void* arg);

// runs a smoker loop that waits, builds, and smokes cigarettes
void* smokerThread(void* arg);

#endif
