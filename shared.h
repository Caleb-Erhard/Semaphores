// shared.h

#ifndef SHARED_H
#define SHARED_H

#include <pthread.h>
#include <semaphore.h>
#include <string>

// constants to make the required loop counts obvious
inline constexpr int AGENT_RUNS = 6;
inline constexpr int PUSHER_RUNS = 12;
inline constexpr int SMOKES_PER_SMOKER = 3;

// shared semaphores used by the agent threads
extern sem_t agentSem;
extern sem_t tobacco;
extern sem_t paper;
extern sem_t matchSem;

// shared semaphore to protect the table state
extern sem_t tableMutex;

// per smoker semaphores
extern sem_t smokerSem[6];

// booleans that represent which ingredient is currently on the table
extern bool isTobacco;
extern bool isPaper;
extern bool isMatch;

// print mutex so thread output does not get squished together on the terminal
extern pthread_mutex_t printMutex;

// this mutex protects the round robin selection among the two smokers of each type
extern pthread_mutex_t chooserMutex;

// these counters let us alternate between the two smokers of a given type
extern int tobaccoChooser;
extern int paperChooser;
extern int matchChooser;

// counters used for the final summary
extern int agentCount[3];
extern int pusherCount[3];
extern int smokerCount[6];

// mutex to protect counter updates
extern pthread_mutex_t counterMutex;

// smoker thread arguments
struct SmokerArgs {
    int id;
    std::string name;
    std::string holding;
};

// agent thread arguments
struct AgentArgs {
    int id;
    std::string name;
    std::string firstIngredient;
    std::string secondIngredient;
};

// pusher thread arguments
struct PusherArgs {
    int id;
    std::string name;
    std::string ingredient;
};

// helper functions
// prints one log line while holding the print mutex
void logLine(const std::string& text);

// sleeps for a random number of milliseconds between 0 and maxMs
void sleepRandomMs(unsigned int& seed, int maxMs);

// builds a consistent log prefix and action message
std::string buildMessage(const std::string& who, const std::string& action);

// posts the semaphore that matches the provided ingredient name
void postIngredient(const std::string& ingredientName);

// wakes one smoker of the requested type using round robin selection
void signalSpecificSmoker(const std::string& smokerType);

// increments the run counter for one agent id
void incrementAgentCount(int id);

// increments the activation counter for one pusher id
void incrementPusherCount(int id);

// increments the completion counter for one smoker id
void incrementSmokerCount(int id);

// prints final execution totals for all thread types
void printSummary();

#endif
