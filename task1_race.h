#ifndef TASK1_RACE_H
#define TASK1_RACE_H

#include <vector>
#include <string>

namespace task1 {
    
    // Основные тесты
    void run_race();
    void run_extended_benchmark();
    
    // Тесты примитивов синхронизации
    void test_mutex(int num_threads, int iterations);
    void test_semaphore(int num_threads, int iterations);
    void test_barrier(int num_threads, int iterations);
    void test_spinlock(int num_threads, int iterations);
    void test_spinwait(int num_threads, int iterations);
    void test_monitor(int num_threads, int iterations);
    
    // Бенчмарк всех примитивов
    void benchmark_all_primitives(int num_threads, int iterations);
    
    // Расширенный бенчмарк с разными параметрами
    void run_scalability_test();
    
} 

#endif 
