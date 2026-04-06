// main.cpp

#include "shared.h"
#include "threads.h"
#include <cstdlib>
#include <ctime>

// initializes shared state, starts all threads, waits for completion, and prints results
int main() {
    // seed the random number generator
    srand(static_cast<unsigned int>(time(nullptr)));

    // initialize semaphores
    // agentSem = 1 means the agent can start immediately, others start at 0 since they need to be signaled by the agent or pushers
    sem_init(&agentSem, 0, 1);
    // tobacco = 0, paper = 0, matchSem = 0 means no ingredient has been posted yet, so the pusher threads will initially block waiting
    sem_init(&tobacco, 0, 0);
    sem_init(&paper, 0, 0);
    sem_init(&matchSem, 0, 0);
    // tableMutex = 1 means the table state is unlocked initially
    sem_init(&tableMutex, 0, 1);

    // initialize smoker semaphores
    sem_init(&smokerSem[0], 0, 0);
    sem_init(&smokerSem[1], 0, 0);
    sem_init(&smokerSem[2], 0, 0);
    sem_init(&smokerSem[3], 0, 0);
    sem_init(&smokerSem[4], 0, 0);
    sem_init(&smokerSem[5], 0, 0);


    // define thread handles
    // act as storage to later refer to the thread when calling
    pthread_t agents[3];
    pthread_t pushers[3];
    pthread_t smokers[6];

    /*
    build readable thread metadata
    Each thread needs to know who it is and what it does to print meaningful log messages

    For example,
    an agent thread needs to know:
        - ID
        - readable name for printing
        - which two ingredients it provides
    a smoker thread needs to know:
        - ID
        - readable name for printing
        - which ingredient it permanently holds
    */
    AgentArgs agentArgs[3] = {
        {0, "AGENT 0", "tobacco", "paper"},
        {1, "AGENT 1", "tobacco", "match"},
        {2, "AGENT 2", "paper", "match"}
    };

    PusherArgs pusherArgs[3] = {
        {0, "PUSHER TOBACCO", "tobacco"},
        {1, "PUSHER PAPER", "paper"},
        {2, "PUSHER MATCH", "match"}
    };

    SmokerArgs smokerArgs[6] = {
        {0, "SMOKER 0", "tobacco"},
        {1, "SMOKER 1", "tobacco"},
        {2, "SMOKER 2", "paper"},
        {3, "SMOKER 3", "paper"},
        {4, "SMOKER 4", "match"},
        {5, "SMOKER 5", "match"}
    };

    logLine("Starting all threads");

    // create agent threads
    pthread_create(&agents[0], nullptr, agentThread, &agentArgs[0]);
    pthread_create(&agents[1], nullptr, agentThread, &agentArgs[1]);
    pthread_create(&agents[2], nullptr, agentThread, &agentArgs[2]);

    // create pusher threads
    pthread_create(&pushers[0], nullptr, tobaccoPusher, &pusherArgs[0]);
    pthread_create(&pushers[1], nullptr, paperPusher, &pusherArgs[1]);
    pthread_create(&pushers[2], nullptr, matchPusher, &pusherArgs[2]);

    // create smoker threads
    pthread_create(&smokers[0], nullptr, smokerThread, &smokerArgs[0]);
    pthread_create(&smokers[1], nullptr, smokerThread, &smokerArgs[1]);
    pthread_create(&smokers[2], nullptr, smokerThread, &smokerArgs[2]);
    pthread_create(&smokers[3], nullptr, smokerThread, &smokerArgs[3]);
    pthread_create(&smokers[4], nullptr, smokerThread, &smokerArgs[4]);
    pthread_create(&smokers[5], nullptr, smokerThread, &smokerArgs[5]);

    // join agent threads
    pthread_join(agents[0], nullptr);
    pthread_join(agents[1], nullptr);
    pthread_join(agents[2], nullptr);

    logLine("All agent threads joined");

    // join pusher threads
    pthread_join(pushers[0], nullptr);
    pthread_join(pushers[1], nullptr);
    pthread_join(pushers[2], nullptr);

    logLine("All pusher threads joined");

    // join smoker threads
    pthread_join(smokers[0], nullptr);
    pthread_join(smokers[1], nullptr);
    pthread_join(smokers[2], nullptr);
    pthread_join(smokers[3], nullptr);
    pthread_join(smokers[4], nullptr);
    pthread_join(smokers[5], nullptr);

    logLine("All smoker threads joined");

    // print final run summary
    printSummary();

    // clean up semaphores
    sem_destroy(&agentSem);
    sem_destroy(&tobacco);
    sem_destroy(&paper);
    sem_destroy(&matchSem);
    sem_destroy(&tableMutex);

    sem_destroy(&smokerSem[0]);
    sem_destroy(&smokerSem[1]);
    sem_destroy(&smokerSem[2]);
    sem_destroy(&smokerSem[3]);
    sem_destroy(&smokerSem[4]);
    sem_destroy(&smokerSem[5]);

    logLine("Program finished");

    pthread_mutex_destroy(&printMutex);
    pthread_mutex_destroy(&chooserMutex);
    pthread_mutex_destroy(&counterMutex);
    
    return 0;
}
