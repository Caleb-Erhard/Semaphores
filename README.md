# Lab 2: POSIX Threads and Semaphores — Cigarette Smokers Problem
 
## Overview
 
This program simulates the **Cigarette Smokers Problem**, a classic operating systems synchronization problem originally proposed by Patil (1971). It is implemented in C++ using POSIX threads (`pthreads`) and POSIX semaphores.
 
The simulation models an operating system scenario where an agent (the OS) distributes resources, and multiple applications (smokers) compete to acquire the resources they need to proceed. The challenge is ensuring that the right application is always woken up when its required resources become available — and that no application is woken up when it cannot proceed.
 
---
 
## The Problem
 
Making a cigarette requires three ingredients: **tobacco**, **paper**, and **match**.
 
There are three types of participants:
 
- **Agents** — have an infinite supply of all three ingredients. Each round, an agent places two different ingredients on the shared table and signals the appropriate pushers.
- **Pushers** — intermediary threads that watch for ingredient arrivals, coordinate which pair has been placed, and wake the correct smoker.
- **Smokers** — each smoker permanently holds one ingredient in infinite supply. A smoker only needs the other two to make a cigarette. Once woken, a smoker picks up the ingredients, makes a cigarette, smokes it, and waits for the next opportunity.
 
The key constraint from the original problem statement is that **the agent code cannot be modified**. The agent simply signals two ingredient semaphores and moves on — it has no knowledge of which smoker to wake. That responsibility falls entirely on the pusher threads, following Parnas' solution.
 
---
 
## Thread Roster
 
| Thread Type | Count | Description |
|---|---|---|
| Agent | 3 | Each places a specific pair of ingredients 6 times |
| Pusher | 3 | One per ingredient type; coordinates smoker wakeups |
| Smoker | 6 | Two per ingredient type; each smokes 3 cigarettes |
 
**Agent assignments:**
- Agent 0 — places tobacco + paper
- Agent 1 — places tobacco + match
- Agent 2 — places paper + match
 
**Smoker assignments:**
- Smokers 0 & 1 — hold tobacco; need paper and match
- Smokers 2 & 3 — hold paper; need tobacco and match
- Smokers 4 & 5 — hold match; need tobacco and paper
 
---
 
## Program Flow
 
### Startup (`main.cpp`)
 
1. All semaphores and mutexes are initialized.
   - `agentSem` starts at **1** so one agent can begin immediately.
   - All ingredient semaphores (`tobacco`, `paper`, `matchSem`) start at **0**.
   - `tableMutex` starts at **1** (unlocked).
   - All six `smokerSem[]` entries start at **0** (smokers begin blocked).
2. All 12 threads are created: 3 agents, 3 pushers, 6 smokers.
3. Main joins agents first, then pushers, then smokers, before printing the summary.
 
---
 
### Agent Threads (`agentThread`)
 
Each agent loops **6 times**. Each iteration:
 
1. Sleeps for a random duration up to **200ms** to introduce scheduling variation.
2. Calls `sem_wait(&agentSem)` — blocks until the table is free.
3. Logs the placement and calls `postIngredient()` twice, once per ingredient.
4. `postIngredient()` calls `sem_post()` on the corresponding ingredient semaphore, waking the appropriate pusher.
 
After 6 rounds, the agent logs its exit and returns.
 
---
 
### Pusher Threads (`tobaccoPusher`, `paperPusher`, `matchPusher`)
 
Each pusher loops **12 times** (6 agents × 2 ingredients per round = 12 total signals per ingredient type across the whole run). Each iteration:
 
1. Calls `sem_wait()` on its ingredient semaphore — blocks until the agent posts that ingredient.
2. Acquires `tableMutex` to safely read and modify the shared boolean table state.
3. Applies the Parnas coordination logic:
   - If the **complementary ingredient** is already flagged on the table → clear that flag and wake the smoker who needs both.
   - If the **other complementary ingredient** is already flagged → clear that flag and wake the smoker who needs both.
   - If **neither** is present → set this ingredient's flag to `true` and wait for the other pusher to arrive.
4. Releases `tableMutex`.
 
The three boolean flags (`isTobacco`, `isPaper`, `isMatch`) represent the current contents of the table. They are only ever modified while `tableMutex` is held, preventing race conditions between pushers.
 
**Example — Agent 1 places tobacco and match:**
1. `tobaccoPusher` wakes, acquires mutex, finds no flags set → sets `isTobacco = true`, releases mutex.
2. `matchPusher` wakes, acquires mutex, finds `isTobacco = true` → clears it, wakes a paper-holding smoker.
 
---
 
### Smoker Wakeup — Round Robin Selection (`signalSpecificSmoker`)
 
Because there are **two smokers per ingredient type**, the pusher must decide which one to wake. A round-robin counter per ingredient type alternates the selection:
 
- Tobacco holders: alternates between Smoker 0 and Smoker 1
- Paper holders: alternates between Smoker 2 and Smoker 3
- Match holders: alternates between Smoker 4 and Smoker 5
 
The chooser counters are protected by `chooserMutex` to prevent concurrent modification.
 
---
 
### Smoker Threads (`smokerThread`)
 
Each smoker loops **3 times**. Each iteration:
 
1. Calls `sem_wait(&smokerSem[id])` — blocks until a pusher wakes it.
2. Logs that it was awakened and has its held ingredient.
3. Simulates making the cigarette by sleeping up to **50ms**.
4. Calls `sem_post(&agentSem)` — signals that the table is free for the next agent round.
5. Simulates smoking by sleeping up to **50ms**.
6. Increments its cigarette completion counter and logs progress.
 
After 3 cigarettes, the smoker logs its exit and returns.
 
---
 
## Semaphore and Mutex Summary
 
| Synchronization Primitive | Type | Purpose |
|---|---|---|
| `agentSem` | Semaphore (init 1) | Controls table access; one agent places at a time; smoker releases it when done |
| `tobacco` | Semaphore (init 0) | Signals the tobacco pusher that tobacco is on the table |
| `paper` | Semaphore (init 0) | Signals the paper pusher that paper is on the table |
| `matchSem` | Semaphore (init 0) | Signals the match pusher that a match is on the table |
| `tableMutex` | Semaphore (init 1) | Mutual exclusion over `isTobacco`, `isPaper`, `isMatch` flags |
| `smokerSem[0..5]` | Semaphores (init 0) | One per smoker; pusher posts to wake the correct smoker |
| `printMutex` | Mutex | Prevents interleaved console output across threads |
| `chooserMutex` | Mutex | Protects round-robin chooser counters |
| `counterMutex` | Mutex | Protects agent, pusher, and smoker run counters |
 
---
 
## Expected Output Totals
 
| Metric | Expected Value | Derivation |
|---|---|---|
| Total agent rounds | 18 | 3 agents × 6 rounds each |
| Total pusher activations | 36 | 3 agents × 6 rounds × 2 ingredients per round |
| Total cigarettes completed | 18 | 6 smokers × 3 cigarettes each |
 
---
 
## File Structure
 
```
.
├── main.cpp       — Entry point; initializes state, creates and joins all threads
├── threads.cpp    — Thread function implementations (agents, pushers, smokers)
├── threads.h      — Thread function declarations
├── shared.cpp     — Shared state, semaphores, mutexes, and helper functions
└── shared.h       — Shared declarations, structs, and constants
```
 
---
 
## Building and Running
 
```bash
g++ -o lab2 main.cpp threads.cpp shared.cpp -lpthread
./lab2
```
 
---