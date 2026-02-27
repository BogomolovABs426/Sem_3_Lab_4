#ifndef TASK3_PHILOSOPHERS_H
#define TASK3_PHILOSOPHERS_H

#include <string>
#include <vector>

namespace task3 {

class DiningPhilosophers {
public:
    enum class Strategy {
        MUTEX,              // Использование мьютексов
        SEMAPHORE,          // Использование семафоров
        TRY_LOCK,           // Попытка захвата вилок
        ARBITRATOR,         // Арбитр (официант)
        RESOURCE_HIERARCHY  // Иерархия ресурсов
    };
    
    DiningPhilosophers(int num_philosophers = 5, Strategy strategy = Strategy::MUTEX);
    void run_simulation(int iterations, bool verbose = true);
    void run_benchmark(int max_philosophers, int iterations);
    
private:
    int num_philosophers_;
    Strategy strategy_;
    
    void philosopher_mutex(int id, int iterations, bool verbose);
    void philosopher_semaphore(int id, int iterations, bool verbose);
    void philosopher_try_lock(int id, int iterations, bool verbose);
    void philosopher_arbitrator(int id, int iterations, bool verbose);
    void philosopher_resource_hierarchy(int id, int iterations, bool verbose);
};

void run_philosophers();
void run_philosophers_benchmark();

} 

#endif 
