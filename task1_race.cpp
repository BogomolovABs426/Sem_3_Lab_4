#include "task1_race.h"
#include "benchmark_utils.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <random>
#include <condition_variable>
#include <sstream>

using namespace std;
using namespace std::chrono_literals;

namespace task1 {

// spin lock
class SpinLock {
private:
    atomic_flag flag = ATOMIC_FLAG_INIT;
    
public:
    void lock() {
        while (flag.test_and_set(memory_order_acquire)) {
            // Активное ожидание
        }
    }
    
    void unlock() {
        flag.clear(memory_order_release);
    }
};

// spinwait
class SpinWait {
private:
    atomic_flag flag = ATOMIC_FLAG_INIT;
    
public:
    void lock() {
        while (flag.test_and_set(memory_order_acquire)) {
            this_thread::yield(); // Уступаем процессорное время
        }
    }
    
    void unlock() {
        flag.clear(memory_order_release);
    }
};

// monitor
class Monitor {
private:
    mutex mtx;
    condition_variable cv;
    bool available = true;
    
public:
    void enter() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]() { return available; });
        available = false;
    }
    
    void exit() {
        lock_guard<mutex> lock(mtx);
        available = true;
        cv.notify_one();
    }
};

// semaphore
class Semaphore {
private:
    mutex mtx;
    condition_variable cv;
    int count;
    
public:
    Semaphore(int initial = 1) : count(initial) {}
    
    void acquire() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]() { return count > 0; });
        count--;
    }
    
    void release() {
        lock_guard<mutex> lock(mtx);
        count++;
        cv.notify_one();
    }
};

// barrier
class Barrier {
private:
    mutex mtx;
    condition_variable cv;
    int count;
    int total;
    int generation = 0;
    
public:
    Barrier(int n) : count(n), total(n) {}
    
    void arrive_and_wait() {
        unique_lock<mutex> lock(mtx);
        int gen = generation;
        
        if (--count == 0) {
            generation++;
            count = total;
            cv.notify_all();
        } else {
            cv.wait(lock, [this, gen]() { return gen != generation; });
        }
    }
};

// Тест mutex
void test_mutex(int num_threads, int iterations) {
    mutex mtx;
    vector<thread> threads;
    atomic<int> counter{0};
    atomic<int> progress{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(33, 126); // Печатные ASCII символы
           
            for (int j = 0; j < iterations; ++j) {
                lock_guard<mutex> lock(mtx);
                char c = static_cast<char>(dis(gen));
                int value = static_cast<int>(c) * (j % 256);
                counter += value % 256;
                progress++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    if (num_threads * iterations < 1000) {
        cout << "  [Mutex] Завершено операций: " << progress.load() 
                  << ", итоговое значение: " << counter.load() << endl;
    }
}

// Тест semaphore
void test_semaphore(int num_threads, int iterations) {
    Semaphore semaphore(1);
    vector<thread> threads;
    atomic<int> counter{0};
    atomic<int> progress{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(33, 126);
            
            for (int j = 0; j < iterations; ++j) {
                semaphore.acquire();
                char c = static_cast<char>(dis(gen));
                int value = static_cast<int>(c) * (j % 256);
                counter += value % 256;
                progress++;
                semaphore.release();
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    if (num_threads * iterations < 1000) {
        cout << "  [Semaphore] Завершено операций: " << progress.load() 
                  << ", итоговое значение: " << counter.load() << endl;
    }
}

// Тест barrier
void test_barrier(int num_threads, int iterations) {
    Barrier sync_point(num_threads);
    vector<thread> threads;
    atomic<int> counter{0};
    atomic<int> progress{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(33, 126);
            
            for (int j = 0; j < iterations; ++j) {
                char c = static_cast<char>(dis(gen));
                int value = static_cast<int>(c) * (j % 256);
                counter += value % 256;
                progress++;
                
                // Синхронизация в барьере(когда все достигли барьера)
                sync_point.arrive_and_wait();
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    if (num_threads * iterations < 1000) {
        cout << "  [Barrier] Завершено операций: " << progress.load() 
                  << ", итоговое значение: " << counter.load() << endl;
    }
}

// Тест spinlock
void test_spinlock(int num_threads, int iterations) {
    SpinLock spinlock;
    vector<thread> threads;
    atomic<int> counter{0};
    atomic<int> progress{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(33, 126);
            
            for (int j = 0; j < iterations; ++j) {
                lock_guard<SpinLock> lock(spinlock);
                char c = static_cast<char>(dis(gen));
                int value = static_cast<int>(c) * (j % 256);
                counter += value % 256;
                progress++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    if (num_threads * iterations < 1000) {
        cout << "  [SpinLock] Завершено операций: " << progress.load() 
                  << ", итоговое значение: " << counter.load() << endl;
    }
}

// Тест spinwait
void test_spinwait(int num_threads, int iterations) {
    SpinWait spinwait;
    vector<thread> threads;
    atomic<int> counter{0};
    atomic<int> progress{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(33, 126);
            
            for (int j = 0; j < iterations; ++j) {
                lock_guard<SpinWait> lock(spinwait);
                char c = static_cast<char>(dis(gen));
                int value = static_cast<int>(c) * (j % 256);
                counter += value % 256;
                progress++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    if (num_threads * iterations < 1000) {
        cout << "  [SpinWait] Завершено операций: " << progress.load() 
                  << ", итоговое значение: " << counter.load() << endl;
    }
}

// Тест monitor
void test_monitor(int num_threads, int iterations) {
    Monitor monitor;
    vector<thread> threads;
    atomic<int> counter{0};
    atomic<int> progress{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(33, 126);
            
            for (int j = 0; j < iterations; ++j) {
                monitor.enter();
                char c = static_cast<char>(dis(gen));
                int value = static_cast<int>(c) * (j % 256);
                counter += value % 256;
                progress++;
                monitor.exit();
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    if (num_threads * iterations < 1000) {
        cout << "  [Monitor] Завершено операций: " << progress.load() 
                  << ", итоговое значение: " << counter.load() << endl;
    }
}

// benchmark all
void benchmark_all_primitives(int num_threads, int iterations) {
    cout << "\n=== Тестирование примитивов синхронизации ===\n";
    cout << "Параметры: " << num_threads << " потоков, " 
              << iterations << " итераций на поток\n";
    cout << "Общее количество операций: " << num_threads * iterations << "\n\n";
    
    vector<pair<string, double>> results;
    
    {
        Benchmark b("Mutex тест", false);
        test_mutex(num_threads, iterations);
        results.emplace_back("Mutex", b.elapsed_microseconds());
    }
    
    {
        Benchmark b("Semaphore тест", false);
        test_semaphore(num_threads, iterations);
        results.emplace_back("Semaphore", b.elapsed_microseconds());
    }
    
    {
        Benchmark b("Barrier тест", false);
        test_barrier(num_threads, iterations);
        results.emplace_back("Barrier", b.elapsed_microseconds());
    }
    
    {
        Benchmark b("SpinLock тест", false);
        test_spinlock(num_threads, iterations);
        results.emplace_back("SpinLock", b.elapsed_microseconds());
    }
    
    {
        Benchmark b("SpinWait тест", false);
        test_spinwait(num_threads, iterations);
        results.emplace_back("SpinWait", b.elapsed_microseconds());
    }
    
    {
        Benchmark b("Monitor тест", false);
        test_monitor(num_threads, iterations);
        results.emplace_back("Monitor", b.elapsed_microseconds());
    }
    
    Benchmark::print_results(results, "Сравнение примитивов синхронизации");
    Benchmark::save_to_csv(results, "primitives_benchmark.csv");
    Benchmark::print_statistics(results);
}

// тест масштабируемости
void run_scalability_test() {
    cout << "\n=== Тест масштабируемости ===\n";
    cout << "Изучаем производительность при разном количестве потоков\n\n";
    
    vector<int> thread_counts = {1, 2, 4, 8};
    const int iterations = 1000;
    
    cout << "Фиксированное количество итераций на поток: " << iterations << "\n";
    cout << "Тестируем примитив: Mutex (как пример)\n\n";
    
    vector<pair<string, double>> scalability_results;
    
    for (int threads : thread_counts) {
        Benchmark b("Масштабируемость: " + to_string(threads) + " потоков", false);
        test_mutex(threads, iterations);
        double time = b.elapsed_microseconds();
        scalability_results.emplace_back(to_string(threads) + " потоков", time);
    }
    
    cout << "\nРезультаты масштабируемости:\n";
    cout << setw(15) << left << "Потоки"
              << setw(15) << "Время (мкс)"
              << setw(15) << "Ускорение" << "\n";
    cout << string(45, '-') << endl;
    
    double base_time = scalability_results[0].second;
    for (const auto& result : scalability_results) {
        double speedup = base_time / result.second;
        cout << setw(15) << left << result.first
                  << setw(15) << fixed << setprecision(2) << result.second
                  << setw(15) << fixed << setprecision(2) << speedup << "x\n";
    }
    cout << string(45, '-') << endl;
}

// расширенный бенчмарк(разные параметры)
void run_extended_benchmark() {
    cout << "\n=== Расширенный бенчмарк примитивов синхронизации ===\n";
    cout << "Выполняем тесты с разными параметрами\n\n";
    
    vector<int> thread_options = {2, 4, 8};
    vector<int> iteration_options = {100, 500, 1000};
    
    vector<pair<string, double>> all_results;
    
    for (int threads : thread_options) {
        for (int iterations : iteration_options) {
            cout << "\n--- Конфигурация: " << threads << " потоков, " 
                      << iterations << " итераций ---\n";
            
            {
                Benchmark b("Mutex", false);
                test_mutex(threads, iterations);
                all_results.emplace_back(
                    "Mutex_" + to_string(threads) + "t_" + to_string(iterations) + "i",
                    b.elapsed_microseconds()
                );
            }
            
            {
                Benchmark b("Semaphore", false);
                test_semaphore(threads, iterations);
                all_results.emplace_back(
                    "Semaphore_" + to_string(threads) + "t_" + to_string(iterations) + "i",
                    b.elapsed_microseconds()
                );
            }
            
            // Для ускорения тестирования, остальные примитивы можно тестировать
            // только при определенных конфигурациях
            if (threads == 4 && iterations == 500) {
                {
                    Benchmark b("Barrier", false);
                    test_barrier(threads, iterations);
                    all_results.emplace_back(
                        "Barrier_" + to_string(threads) + "t_" + to_string(iterations) + "i",
                        b.elapsed_microseconds()
                    );
                }
                
                {
                    Benchmark b("SpinLock", false);
                    test_spinlock(threads, iterations);
                    all_results.emplace_back(
                        "SpinLock_" + to_string(threads) + "t_" + to_string(iterations) + "i",
                        b.elapsed_microseconds()
                    );
                }
            }
        }
    }
    
    Benchmark::save_to_csv(all_results, "extended_benchmark.csv");
    cout << "\nРасширенный бенчмарк завершен. Результаты сохранены в extended_benchmark.csv\n";
}

// основная
void run_race() {
    cout << "\n=== Задание 1: Параллельная гонка с ASCII символами ===\n";
    cout << "Сравнение 6 примитивов синхронизации:\n";
    cout << "1. Mutex (взаимное исключение)\n";
    cout << "2. Semaphore (семафор)\n";
    cout << "3. Barrier (барьер)\n";
    cout << "4. SpinLock (спин-блокировка)\n";
    cout << "5. SpinWait (ожидание с уступкой)\n";
    cout << "6. Monitor (монитор)\n\n";
    
    int choice;
    cout << "Выберите режим тестирования:\n";
    cout << "1. Стандартный тест (все примитивы с заданными параметрами)\n";
    cout << "2. Тест масштабируемости\n";
    cout << "3. Расширенный бенчмарк\n";
    cout << "Ваш выбор: ";
    cin >> choice;
    
    switch (choice) {
        case 1: {
            int num_threads, iterations;
            
            cout << "\nВведите количество потоков (1-16): ";
            cin >> num_threads;
            
            cout << "Введите количество итераций на поток (100-10000): ";
            cin >> iterations;
            
            if (num_threads < 1 || num_threads > 16 || iterations < 100 || iterations > 10000) {
                cout << "Некорректные параметры! Использую значения по умолчанию.\n";
                num_threads = 4;
                iterations = 1000;
            }
            
            benchmark_all_primitives(num_threads, iterations);
            break;
        }
        case 2:
            run_scalability_test();
            break;
        case 3:
            run_extended_benchmark();
            break;
        default:
            cout << "Неверный выбор! Запускаю стандартный тест...\n";
            benchmark_all_primitives(4, 1000);
    }
}

} 
