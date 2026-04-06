// threads.cpp

#include "threads.h"
#include "shared.h"
#include <ctime>
#include <sstream>

// runs an agent loop that places two ingredients and wakes pushers
void* agentThread(void* arg) {
    // unpack typed agent data from the generic pthread argument
    AgentArgs* info = static_cast<AgentArgs*>(arg);

    // build a per thread pseudo random seed using time and the agent id
    unsigned int seed = static_cast<unsigned int>(time(nullptr)) ^ (info->id * 12345);

    // each agent performs a fixed number of table placements
    for (int i = 0; i < AGENT_RUNS; i++) {
        // wait a little so output and scheduling are easier to observe
        sleepRandomMs(seed, 200);

        // wait for the table to become free before placing ingredients
        sem_wait(&agentSem);
        // track how many times this agent has run
        incrementAgentCount(info->id);

        {
            // detailed round message for traceability
            std::ostringstream out;
            out << info->name << " placed " << info->firstIngredient << " and " << info->secondIngredient << " on the table (round " << (i + 1) << "/" << AGENT_RUNS << ")";
            logLine(out.str());
        }

        // wake pushers for each produced ingredient
        postIngredient(info->firstIngredient);
        postIngredient(info->secondIngredient);
    }

    // log final lifecycle event for this thread
    logLine(buildMessage(info->name, "finished all rounds and exits"));
    return nullptr;
}

// handles tobacco arrivals and wakes the matching smoker when a pair is complete
void* tobaccoPusher(void* arg) {
    // unpack pusher metadata
    PusherArgs* info = static_cast<PusherArgs*>(arg);

    // handle one tobacco arrival per loop iteration
    for (int i = 0; i < PUSHER_RUNS; i++) {
        // wait until tobacco appears on the table
        sem_wait(&tobacco);
        // count work performed by this pusher
        incrementPusherCount(info->id);
        // protect shared table state flags
        sem_wait(&tableMutex);

        // if paper is waiting then tobacco and paper is now complete
        if (isPaper) {
            isPaper = false;
            logLine(buildMessage(info->name, "saw tobacco and paper, so a smoker with match is needed"));
            signalSpecificSmoker("match");
        // if match is waiting then tobacco and match is now complete
        } else if (isMatch) {
            isMatch = false;
            logLine(buildMessage(info->name, "saw tobacco and match, so a smoker with paper is needed"));
            signalSpecificSmoker("paper");
        } else {
            // otherwise remember that tobacco is currently waiting
            isTobacco = true;
            logLine(buildMessage(info->name, "stored tobacco on the table state"));
        }

        // release shared table state lock
        sem_post(&tableMutex);
    }

    // log final lifecycle event for this thread
    logLine(buildMessage(info->name, "finished all rounds and exits"));
    return nullptr;
}

// handles paper arrivals and wakes the matching smoker when a pair is complete
void* paperPusher(void* arg) {
    // unpack pusher metadata
    PusherArgs* info = static_cast<PusherArgs*>(arg);

    // handle one paper arrival per loop iteration
    for (int i = 0; i < PUSHER_RUNS; i++) {
        // wait until paper appears on the table
        sem_wait(&paper);
        // count work performed by this pusher
        incrementPusherCount(info->id);
        // protect shared table state flags
        sem_wait(&tableMutex);

        // if tobacco is waiting then paper and tobacco is now complete
        if (isTobacco) {
            isTobacco = false;
            logLine(buildMessage(info->name, "saw paper and tobacco, so a smoker with match is needed"));
            signalSpecificSmoker("match");
        // if match is waiting then paper and match is now complete
        } else if (isMatch) {
            isMatch = false;
            logLine(buildMessage(info->name, "saw paper and match, so a smoker with tobacco is needed"));
            signalSpecificSmoker("tobacco");
        } else {
            // otherwise remember that paper is currently waiting
            isPaper = true;
            logLine(buildMessage(info->name, "stored paper on the table state"));
        }

        // release shared table state lock
        sem_post(&tableMutex);
    }

    // log final lifecycle event for this thread
    logLine(buildMessage(info->name, "finished all rounds and exits"));
    
    return nullptr;
}

// handles match arrivals and wakes the matching smoker when a pair is complete
void* matchPusher(void* arg) {
    // unpack pusher metadata
    PusherArgs* info = static_cast<PusherArgs*>(arg);

    // handle one match arrival per loop iteration
    for (int i = 0; i < PUSHER_RUNS; i++) {
        // wait until match appears on the table
        sem_wait(&matchSem);
        // count work performed by this pusher
        incrementPusherCount(info->id);
        // protect shared table state flags
        sem_wait(&tableMutex);

        // if tobacco is waiting then match and tobacco is now complete
        if (isTobacco) {
            isTobacco = false;
            logLine(buildMessage(info->name, "saw match and tobacco, so a smoker with paper is needed"));
            signalSpecificSmoker("paper");
        // if paper is waiting then match and paper is now complete
        } else if (isPaper) {
            isPaper = false;
            logLine(buildMessage(info->name, "saw match and paper, so a smoker with tobacco is needed"));
            signalSpecificSmoker("tobacco");
        } else {
            // otherwise remember that match is currently waiting
            isMatch = true;
            logLine(buildMessage(info->name, "stored match on the table state"));
        }

        // release shared table state lock
        sem_post(&tableMutex);
    }

    // log final event for this thread
    logLine(buildMessage(info->name, "finished all rounds and exits"));
    
    return nullptr;
}

// runs a smoker loop that waits, builds, and smokes cigarettes
void* smokerThread(void* arg) {
    // unpack typed smoker data from the generic pthread argument
    SmokerArgs* info = static_cast<SmokerArgs*>(arg);

    // build a per thread pseudo random seed using time and smoker id
    unsigned int seed = static_cast<unsigned int>(time(nullptr)) ^ (info->id * 54321);

    // each smoker consumes a fixed number of opportunities to smoke
    for (int i = 0; i < SMOKES_PER_SMOKER; i++) {
        // block until pushers identify this smoker as the one to wake
        sem_wait(&smokerSem[info->id]);

        {
            // record why this smoker can continue
            std::ostringstream out;
            out << info->name << " was awakened and has " << info->holding << ", so the other two ingredients must be available";
            logLine(out.str());
        }

        // simulate cigarette construction work
        logLine(buildMessage(info->name, "starts making a cigarette"));
        sleepRandomMs(seed, 50);

        // notify an agent that the table can be used again
        logLine(buildMessage(info->name, "finished making a cigarette and signals the agent"));
        sem_post(&agentSem);

        // simulate smoking work
        logLine(buildMessage(info->name, "starts smoking"));
        sleepRandomMs(seed, 50);

        // count completed cigarettes by this smoker
        incrementSmokerCount(info->id);

        {
            // emit progress output for this smoker
            std::ostringstream out;
            out << info->name << " finished cigarette " << (i + 1) << "/" << SMOKES_PER_SMOKER;
            logLine(out.str());
        }
    }

    // log final event for this thread
    logLine(buildMessage(info->name, "gets a bit hungry, leaves, and exits"));
    
    return nullptr;
}
