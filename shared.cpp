// shared.cpp

#include "shared.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <unistd.h>

sem_t agentSem;
sem_t tobacco;
sem_t paper;
sem_t matchSem;

// mutex like semaphore that protects table flag updates
sem_t tableMutex;
// one wakeup semaphore per smoker thread
sem_t smokerSem[6];

// table presence flags set and cleared by pusher coordination
bool isTobacco = false;
bool isPaper = false;
bool isMatch = false;

// protects console output so lines do not interleave
pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;
// protects smoker chooser counters used for round robin wakeups
pthread_mutex_t chooserMutex = PTHREAD_MUTEX_INITIALIZER;

// chooser counters used to alternate between the two smokers per ingredient type
int tobaccoChooser = 0;
int paperChooser = 0;
int matchChooser = 0;

// execution counters used in the final summary report
int agentCount[3] = {0, 0, 0};
int pusherCount[3] = {0, 0, 0};
int smokerCount[6] = {0, 0, 0, 0, 0, 0};

// protects all counter increments
pthread_mutex_t counterMutex = PTHREAD_MUTEX_INITIALIZER;

// prints one log line while holding the print mutex
void logLine(const std::string& text) {
    // take print lock before writing shared stdout
    pthread_mutex_lock(&printMutex);
    std::cout << text << std::endl;
    // release lock after one full line is printed
    pthread_mutex_unlock(&printMutex);
}

// sleeps for a random number of milliseconds between 0 and maxMs
void sleepRandomMs(unsigned int& seed, int maxMs) {
    // generate a bounded pseudo random delay in milliseconds
    int delay = rand_r(&seed) % (maxMs + 1);
    // convert milliseconds to microseconds for usleep
    usleep(delay * 1000);
}

// builds a consistent log prefix and action message
std::string buildMessage(const std::string& who, const std::string& action) {
    // build messages in one place so output format stays consistent
    std::ostringstream out;
    out << who << ": " << action;
    return out.str();
}

// posts the semaphore that matches the provided ingredient name
void postIngredient(const std::string& ingredientName) {
    // forward tobacco arrivals to the tobacco pusher
    if (ingredientName == "tobacco") {
        sem_post(&tobacco);
    // forward paper arrivals to the paper pusher
    } else if (ingredientName == "paper") {
        sem_post(&paper);
    // forward match arrivals to the match pusher
    } else if (ingredientName == "match") {
        sem_post(&matchSem);
    }
}

// wakes one smoker of the requested type using round robin selection
void signalSpecificSmoker(const std::string& smokerType) {
    // serialize chooser updates so target selection is deterministic
    pthread_mutex_lock(&chooserMutex);

    // tobacco holders are smoker 0 and smoker 1
    if (smokerType == "tobacco") {
        // alternate target each time this type is requested
        int target = (tobaccoChooser % 2 == 0) ? 0 : 1;
        tobaccoChooser++;
        // wake the selected smoker thread
        sem_post(&smokerSem[target]);

        // log chooser behavior for debugging and tracing
        std::ostringstream out;
        out << "PUSHER LOGIC: waking smoker " << target << " (has tobacco, needs paper and match)";
        logLine(out.str());
    // paper holders are smoker 2 and smoker 3
    } else if (smokerType == "paper") {
        // alternate target each time this type is requested
        int target = (paperChooser % 2 == 0) ? 2 : 3;
        paperChooser++;
        // wake the selected smoker thread
        sem_post(&smokerSem[target]);

        // log chooser behavior for debugging and tracing
        std::ostringstream out;
        out << "PUSHER LOGIC: waking smoker " << target << " (has paper, needs tobacco and match)";
        logLine(out.str());
    // match holders are smoker 4 and smoker 5
    } else if (smokerType == "match") {
        // alternate target each time this type is requested
        int target = (matchChooser % 2 == 0) ? 4 : 5;
        matchChooser++;
        // wake the selected smoker thread
        sem_post(&smokerSem[target]);

        // log chooser behavior for debugging and tracing
        std::ostringstream out;
        out << "PUSHER LOGIC: waking smoker " << target << " (has match, needs tobacco and paper)";
        logLine(out.str());
    }

    // release chooser lock after selection and wakeup
    pthread_mutex_unlock(&chooserMutex);
}

// increments the run counter for one agent id
void incrementAgentCount(int id) {
    // lock shared counters before incrementing
    pthread_mutex_lock(&counterMutex);
    agentCount[id]++;
    // unlock after update
    pthread_mutex_unlock(&counterMutex);
}

// increments the activation counter for one pusher id
void incrementPusherCount(int id) {
    // lock shared counters before incrementing
    pthread_mutex_lock(&counterMutex);
    pusherCount[id]++;
    // unlock after update
    pthread_mutex_unlock(&counterMutex);
}

// increments the completion counter for one smoker id
void incrementSmokerCount(int id) {
    // lock shared counters before incrementing
    pthread_mutex_lock(&counterMutex);
    smokerCount[id]++;
    // unlock after update
    pthread_mutex_unlock(&counterMutex);
}

// prints final execution totals for all thread types
void printSummary() {
    // hold print lock for the full summary block to keep it contiguous
    pthread_mutex_lock(&printMutex);

    // print per agent execution totals
    std::cout << "\n---------- SUMMARY ----------\n";
    std::cout << "AGENT EXECUTION COUNTS:\n";
    for (int i = 0; i < 3; i++) {
        std::cout << "AGENT " << i << " ran " << agentCount[i] << " times\n";
    }

    // print per pusher activation totals
    std::cout << "\nPUSHER EXECUTION COUNTS:\n";
    std::cout << "PUSHER TOBACCO ran " << pusherCount[0] << " times\n";
    std::cout << "PUSHER PAPER ran " << pusherCount[1] << " times\n";
    std::cout << "PUSHER MATCH ran " << pusherCount[2] << " times\n";

    // print per smoker completion totals
    std::cout << "\nSMOKER COMPLETION COUNTS:\n";
    for (int i = 0; i < 6; i++) {
        std::cout << "SMOKER " << i << " completed " << smokerCount[i] << " cigarettes\n";
    }

    // accumulate totals for quick correctness checks
    int totalAgentRounds = 0;
    int totalPusherRounds = 0;
    int totalSmokerRounds = 0;

    // sum agent and pusher counters
    for (int i = 0; i < 3; i++) {
        totalAgentRounds += agentCount[i];
        totalPusherRounds += pusherCount[i];
    }

    // sum smoker counters
    for (int i = 0; i < 6; i++) {
        totalSmokerRounds += smokerCount[i];
    }

    // print computed totals
    std::cout << "\nTOTALS LIST:\n";
    std::cout << "TOTAL AGENT ROUNDS = " << totalAgentRounds << "\n";
    std::cout << "TOTAL PUSHER ACTIVATIONS = " << totalPusherRounds << "\n";
    std::cout << "TOTAL CIGARETTES COMPLETED = " << totalSmokerRounds << "\n";

    // print expected totals for manual comparison
    std::cout << "\nEXPECTED VALUES:\n";
    std::cout << "EXPECTED TOTAL AGENT ROUNDS = 18\n";
    std::cout << "EXPECTED TOTAL PUSHER ACTIVATIONS = 36\n";
    std::cout << "EXPECTED TOTAL CIGARETTES COMPLETED = 18\n";
    std::cout << "----------------------------------------\n";

    // release print lock after summary is complete
    pthread_mutex_unlock(&printMutex);
}
