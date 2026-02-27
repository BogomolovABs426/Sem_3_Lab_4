#include "task3_philosophers.h"
#include "benchmark_utils.h"
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <random>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <condition_variable>
#include <atomic>

using namespace std;
using namespace std::chrono_literals;

namespace task3 {

// Семафор
class BinarySemaphore {
private:
    mutex mtx;
    condition_variable cv;
    bool available;
    
public:
    BinarySemaphore(bool initial = true) : available(initial) {}
    
    void acquire() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]() { return available; });
        available = false;
    }
    
    void release() {
        {
            lock_guard<mutex> lock(mtx);
            available = true;
        }
        cv.notify_one();
    }
};

class DiningPhilosophersImpl {
public:
    static vector<mutex> forks;
    static vector<BinarySemaphore> sem_forks;
    static mutex arbitrator_mutex;
    static vector<bool> forks_available;
    static condition_variable arbitrator_cv;
    static mutex cout_mutex;
    static atomic<bool> simulation_active;
};


vector<mutex> DiningPhilosophersImpl::forks;
vector<BinarySemaphore> DiningPhilosophersImpl::sem_forks;
mutex DiningPhilosophersImpl::arbitrator_mutex;
vector<bool> DiningPhilosophersImpl::forks_available;
condition_variable DiningPhilosophersImpl::arbitrator_cv;
mutex DiningPhilosophersImpl::cout_mutex;
atomic<bool> DiningPhilosophersImpl::simulation_active{true};

DiningPhilosophers::DiningPhilosophers(int num_philosophers, Strategy strategy)
    : num_philosophers_(num_philosophers), strategy_(strategy) {
    // Инициализируем статические переменные
    DiningPhilosophersImpl::forks = vector<mutex>(num_philosophers);
    DiningPhilosophersImpl::sem_forks = vector<BinarySemaphore>(num_philosophers);
    DiningPhilosophersImpl::forks_available = vector<bool>(num_philosophers, true);
}

void DiningPhilosophers::philosopher_mutex(int id, int iterations, bool verbose) {
    auto& forks = DiningPhilosophersImpl::forks;
    auto& cout_mutex = DiningPhilosophersImpl::cout_mutex;
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> think_dist(50, 200);
    uniform_int_distribution<> eat_dist(100, 300);
    
    int left_fork = id;
    int right_fork = (id + 1) % num_philosophers_;
    
    for (int i = 0; i < iterations; ++i) {
        // Захват вилок - гарантируем порядок для избежания deadlock
        if (id % 2 == 0) {
            // Четные философы берут сначала левую, потом правую
            forks[left_fork].lock();
            forks[right_fork].lock();
        } else {
            // Нечетные философы берут сначала правую, потом левую
            forks[right_fork].lock();
            forks[left_fork].lock();
        }
        
        if (verbose) {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " ест спагетти (итерация " << i + 1 << ")\n";
        }
        
        // Еда
        this_thread::sleep_for(chrono::milliseconds(eat_dist(gen)));
        
        // Освобождение вилок
        forks[left_fork].unlock();
        forks[right_fork].unlock();
        
        if (verbose) {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " размышляет (итерация " << i + 1 << ")\n";
        }
        
        // Размышление
        this_thread::sleep_for(chrono::milliseconds(think_dist(gen)));
    }
}

void DiningPhilosophers::philosopher_semaphore(int id, int iterations, bool verbose) {
    auto& sem_forks = DiningPhilosophersImpl::sem_forks;
    auto& cout_mutex = DiningPhilosophersImpl::cout_mutex;
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> think_dist(50, 200);
    uniform_int_distribution<> eat_dist(100, 300);
    
    int left_fork = id;
    int right_fork = (id + 1) % num_philosophers_;
    
    for (int i = 0; i < iterations; ++i) {
        // Захват вилок с правильным порядком для избежания deadlock
        if (id % 2 == 0) {
            sem_forks[left_fork].acquire();
            sem_forks[right_fork].acquire();
        } else {
            sem_forks[right_fork].acquire();
            sem_forks[left_fork].acquire();
        }
        
        if (verbose) {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " ест спагетти (итерация " << i + 1 << ")\n";
        }
        
        // Еда
        this_thread::sleep_for(chrono::milliseconds(eat_dist(gen)));
        
        // Освобождение вилок
        sem_forks[left_fork].release();
        sem_forks[right_fork].release();
        
        if (verbose) {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " размышляет (итерация " << i + 1 << ")\n";
        }
        
        // Размышление
        this_thread::sleep_for(chrono::milliseconds(think_dist(gen)));
    }
}

void DiningPhilosophers::philosopher_try_lock(int id, int iterations, bool verbose) {
    auto& forks = DiningPhilosophersImpl::forks;
    auto& cout_mutex = DiningPhilosophersImpl::cout_mutex;
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> think_dist(50, 200);
    uniform_int_distribution<> eat_dist(100, 300);
    uniform_int_distribution<> retry_dist(10, 50);
    
    int left_fork = id;
    int right_fork = (id + 1) % num_philosophers_;
    
    for (int i = 0; i < iterations; ++i) {
        // Попытка захвата вилок с повторными попытками
        bool has_forks = false;
        int attempts = 0;
        while (!has_forks && attempts < 100) {
            attempts++;
            
            // Пытаемся захватить левую вилку
            if (forks[left_fork].try_lock()) {
                // Если захватили левую, пробуем правую
                if (forks[right_fork].try_lock()) {
                    has_forks = true;
                } else {
                    // Не удалось захватить правую - отпускаем левую
                    forks[left_fork].unlock();
                    this_thread::sleep_for(chrono::milliseconds(retry_dist(gen)));
                }
            } else {
                // Не удалось захватить левую - ждем
                this_thread::sleep_for(chrono::milliseconds(retry_dist(gen)));
            }
        }
        
        // Если не удалось захватить вилки после многих попыток, используем блокирующий захват
        if (!has_forks) {
            forks[left_fork].lock();
            forks[right_fork].lock();
        }
        
        if (verbose) {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " ест спагетти (итерация " << i + 1 << ")\n";
        }
        
        // Еда
        this_thread::sleep_for(chrono::milliseconds(eat_dist(gen)));
        
        // Освобождение вилок
        forks[left_fork].unlock();
        forks[right_fork].unlock();
        
        if (verbose) {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " размышляет (итерация " << i + 1 << ")\n";
        }
        
        // Размышление
        this_thread::sleep_for(chrono::milliseconds(think_dist(gen)));
    }
}

void DiningPhilosophers::philosopher_arbitrator(int id, int iterations, bool verbose) {
    auto& arbitrator_mutex = DiningPhilosophersImpl::arbitrator_mutex;
    auto& forks_available = DiningPhilosophersImpl::forks_available;
    auto& arbitrator_cv = DiningPhilosophersImpl::arbitrator_cv;
    auto& cout_mutex = DiningPhilosophersImpl::cout_mutex;
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> think_dist(50, 200);
    uniform_int_distribution<> eat_dist(100, 300);
    
    int left_fork = id;
    int right_fork = (id + 1) % num_philosophers_;
    
    for (int i = 0; i < iterations; ++i) {
        // Запрос разрешения у арбитра
        {
            unique_lock<mutex> lock(arbitrator_mutex);
            // Ждем, пока обе вилки не станут доступны
            arbitrator_cv.wait(lock, [&]() { 
                return forks_available[left_fork] && forks_available[right_fork]; 
            });
            
            // Захватываем вилки
            forks_available[left_fork] = false;
            forks_available[right_fork] = false;
        }
        
        if (verbose) {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " ест спагетти (итерация " << i + 1 << ")\n";
        }
        
        // Еда
        this_thread::sleep_for(chrono::milliseconds(eat_dist(gen)));
        
        // Возврат вилок арбитру
        {
            lock_guard<mutex> lock(arbitrator_mutex);
            forks_available[left_fork] = true;
            forks_available[right_fork] = true;
            arbitrator_cv.notify_all(); // Уведомляем всех ожидающих
        }
        
        if (verbose) {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " размышляет (итерация " << i + 1 << ")\n";
        }
        
        // Размышление
        this_thread::sleep_for(chrono::milliseconds(think_dist(gen)));
    }
}

void DiningPhilosophers::philosopher_resource_hierarchy(int id, int iterations, bool verbose) {
    auto& forks = DiningPhilosophersImpl::forks;
    auto& cout_mutex = DiningPhilosophersImpl::cout_mutex;
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> think_dist(50, 200);
    uniform_int_distribution<> eat_dist(100, 300);
    
    int left_fork = id;
    int right_fork = (id + 1) % num_philosophers_;
    
    // Все философы берут вилки в порядке возрастания номеров
    int first_fork = min(left_fork, right_fork);
    int second_fork = max(left_fork, right_fork);
    
    for (int i = 0; i < iterations; ++i) {
        // Захват вилок в порядке возрастания номеров
        forks[first_fork].lock();
        forks[second_fork].lock();
        
        if (verbose) {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " ест спагетти (итерация " << i + 1 << ")\n";
        }
        
        // Еда
        this_thread::sleep_for(chrono::milliseconds(eat_dist(gen)));
        
        // Освобождение вилок (в обратном порядке)
        forks[second_fork].unlock();
        forks[first_fork].unlock();
        
        if (verbose) {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " размышляет (итерация " << i + 1 << ")\n";
        }
        
        // Размышление
        this_thread::sleep_for(chrono::milliseconds(think_dist(gen)));
    }
}

void DiningPhilosophers::run_simulation(int iterations, bool verbose) {
    vector<thread> philosophers;
    
    string strategy_name;
    switch (strategy_) {
        case Strategy::MUTEX: strategy_name = "Мьютексы"; break;
        case Strategy::SEMAPHORE: strategy_name = "Семафоры"; break;
        case Strategy::TRY_LOCK: strategy_name = "Попытка захвата"; break;
        case Strategy::ARBITRATOR: strategy_name = "Арбитр "; break;
        case Strategy::RESOURCE_HIERARCHY: strategy_name = "Иерархия ресурсов"; break;
    }
    
    cout << "\n=== Задача обедающих философов ===\n";
    cout << "Философов: " << num_philosophers_ << "\n";
    cout << "Стратегия: " << strategy_name << "\n";
    cout << "Итераций: " << iterations << "\n";
    
    
    if (verbose && iterations > 10) {
        cout << "(Вывод ограничен первыми 10 итерациями)\n";
    }
    
    // Запуск философов
    for (int i = 0; i < num_philosophers_; ++i) {
        switch (strategy_) {
            case Strategy::MUTEX:
                philosophers.emplace_back(&DiningPhilosophers::philosopher_mutex, 
                                         this, i, iterations, verbose);
                break;
            case Strategy::SEMAPHORE:
                philosophers.emplace_back(&DiningPhilosophers::philosopher_semaphore, 
                                         this, i, iterations, verbose);
                break;
            case Strategy::TRY_LOCK:
                philosophers.emplace_back(&DiningPhilosophers::philosopher_try_lock, 
                                         this, i, iterations, verbose);
                break;
            case Strategy::ARBITRATOR:
                philosophers.emplace_back(&DiningPhilosophers::philosopher_arbitrator, 
                                         this, i, iterations, verbose);
                break;
            case Strategy::RESOURCE_HIERARCHY:
                philosophers.emplace_back(&DiningPhilosophers::philosopher_resource_hierarchy, 
                                         this, i, iterations, verbose);
                break;
        }
    }
    
    // Ожидание завершения
    for (auto& p : philosophers) {
        p.join();
    }
    
    cout << "\nСимуляция завершена успешно!\n";
}

void DiningPhilosophers::run_benchmark(int max_philosophers, int iterations) {
    cout << "\n=== Бенчмарк задачи обедающих философов ===\n";
    cout << "Тестируем разные стратегии и количество философов\n\n";
    
    vector<pair<string, double>> benchmark_results;
    
    vector<Strategy> strategies = {
        Strategy::MUTEX,
        Strategy::SEMAPHORE,
        Strategy::TRY_LOCK,
        Strategy::ARBITRATOR,
        Strategy::RESOURCE_HIERARCHY
    };
    
    vector<string> strategy_names = {
        "Мьютексы",
        "Семафоры",
        "Попытка захвата",
        "Арбитр",
        "Иерархия ресурсов"
    };
    
    vector<int> philosopher_counts = {5, 10, 20};
    
    for (int count : philosopher_counts) {
        if (count > max_philosophers) continue;
        
        for (size_t s = 0; s < strategies.size(); ++s) {
            string test_name = to_string(count) + "_философов_" + strategy_names[s];
            
            cout << "Тестируем: " << test_name << "... ";
            cout.flush();
            
            DiningPhilosophers dp(count, strategies[s]);
            
            try {
                Benchmark b(test_name, false);
                dp.run_simulation(iterations, false);
                
                double time = b.elapsed_microseconds();
                benchmark_results.emplace_back(test_name, time);
                
                cout << time << " мкс\n";
            } catch (const exception& e) {
                cout << "ОШИБКА: " << e.what() << "\n";
            }
        }
    }
    
    Benchmark::save_to_csv(benchmark_results, "philosophers_benchmark.csv");
    cout << "\nБенчмарк завершен. Результаты сохранены в philosophers_benchmark.csv\n";
}

void run_philosophers() {
    cout << "\n=== Задание 3: Обедающие философы ===\n";
    cout << "Классическая задача синхронизации\n\n";
    
    int choice;
    cout << "Выберите режим:\n";
    cout << "1. Стандартная симуляция\n";
    cout << "2. Расширенный бенчмарк\n";
    cout << "Ваш выбор: ";
    cin >> choice;
    
    switch (choice) {
        case 1: {
            int num_philosophers, iterations, strategy_choice;
            
            cout << "\nВведите количество философов (2-20): ";
            cin >> num_philosophers;
            
            cout << "Введите количество итераций на философа (1-100): ";
            cin >> iterations;
            
            cout << "\nВыберите стратегию синхронизации:\n";
            cout << "1. Мьютексы\n";
            cout << "2. Семафоры\n";
            cout << "3. Попытка захвата (try_lock)\n";
            cout << "4. Арбитр \n";
            cout << "5. Иерархия ресурсов \n";
            cout << "Ваш выбор: ";
            cin >> strategy_choice;
            
            if (num_philosophers < 2) num_philosophers = 2;
            if (num_philosophers > 20) num_philosophers = 20;
            if (iterations < 1) iterations = 1;
            if (iterations > 100) iterations = 100;
            
            DiningPhilosophers::Strategy strategy;
            switch (strategy_choice) {
                case 1: strategy = DiningPhilosophers::Strategy::MUTEX; break;
                case 2: strategy = DiningPhilosophers::Strategy::SEMAPHORE; break;
                case 3: strategy = DiningPhilosophers::Strategy::TRY_LOCK; break;
                case 4: strategy = DiningPhilosophers::Strategy::ARBITRATOR; break;
                case 5: strategy = DiningPhilosophers::Strategy::RESOURCE_HIERARCHY; break;
                default: strategy = DiningPhilosophers::Strategy::RESOURCE_HIERARCHY;
            }
            
            DiningPhilosophers dp(num_philosophers, strategy);
            
            Benchmark b("Симуляция обедающих философов");
            dp.run_simulation(iterations, true);
            break;
        }
        case 2: {
            int iterations;
            cout << "\nВведите количество итераций на философа (10-100): ";
            cin >> iterations;
            
            if (iterations < 10) iterations = 10;
            if (iterations > 100) iterations = 100;
            
            DiningPhilosophers dp(5, DiningPhilosophers::Strategy::MUTEX);
            dp.run_benchmark(20, iterations);
            break;
        }
        default:
            cout << "Неверный выбор! Запускаю стандартную симуляцию...\n";
            DiningPhilosophers dp(5, DiningPhilosophers::Strategy::RESOURCE_HIERARCHY);
            dp.run_simulation(10, true);
    }
}

void run_philosophers_benchmark() {
    cout << "\n=== Расширенный бенчмарк обедающих философов ===\n";
    
    int iterations;
    cout << "Введите количество итераций на философа (10-100): ";
    cin >> iterations;
    
    if (iterations < 10) iterations = 10;
    if (iterations > 100) iterations = 100;
    
    cout << "\nТестируем все стратегии...\n";
    
    DiningPhilosophers dp(5, DiningPhilosophers::Strategy::MUTEX);
    dp.run_benchmark(20, iterations);
}

}
