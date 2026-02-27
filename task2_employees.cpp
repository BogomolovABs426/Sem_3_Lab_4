#include "task2_employees.h"
#include "benchmark_utils.h"
#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <algorithm>
#include <mutex>
#include <iomanip>
#include <cmath>
#include <sstream>

using namespace std;
namespace task2 {


// Генерация сотрудников	
vector<Employee> generate_employees(int count, const string& target_position) {
    vector<Employee> employees;
    random_device rd;
    mt19937 gen(rd());
    
    // Списки для генерации данных
    vector<string> first_names = {"Иван", "Петр", "Сергей", "Алексей", "Дмитрий", 
                                           "Мария", "Ольга", "Елена", "Анна", "Наталья"};
    vector<string> last_names = {"Иванов", "Петров", "Сидоров", "Смирнов", "Кузнецов",
                                          "Попов", "Васильев", "Павлов", "Семенов", "Федоров"};
    vector<string> middle_names = {"Иванович", "Петрович", "Сергеевич", "Алексеевич", 
                                            "Дмитриевич", "Ивановна", "Петровна", "Сергеевна", 
                                            "Алексеевна", "Дмитриевна"};
    
    vector<string> positions = {"Менеджер", "Разработчик", "Аналитик", "Тестировщик", 
                                         "Дизайнер", "Администратор", "Бухгалтер", target_position};
    // Диапазоны всего
    uniform_int_distribution<> age_dist(20, 65);
    uniform_real_distribution<> salary_dist(30000, 300000);
    uniform_int_distribution<> position_dist(0, positions.size() - 1);
    
    for (int i = 0; i < count; ++i) {
        // Генерация ФИО
        ostringstream name;
        name << last_names[gen() % last_names.size()] << " "
             << first_names[gen() % first_names.size()] << " "
             << middle_names[gen() % middle_names.size()];
        
        // Генерация должности
        string position = positions[position_dist(gen)];
        
        // Генерация возраста
        int age = age_dist(gen);
        
        // Генерация зарплаты
        double salary = salary_dist(gen);
        
        employees.emplace_back(name.str(), position, age, salary);
    }
    
    // Проверка сотрудников с целевой должностью
    bool has_target_position = false;
    for (const auto& emp : employees) {
        if (emp.position == target_position) {
            has_target_position = true;
            break;
        }
    }
    // Иначе присваивается первому сотруднику
    if (!has_target_position && !employees.empty()) {
        employees[0].position = target_position;
    }
    
    return employees;
}

// Расчет среднего возраста
double calculate_average_age(const vector<Employee>& employees, const string& target_position) {
    double total_age = 0.0;
    int count = 0;
    
    for (const auto& emp : employees) {
        if (emp.position == target_position) {
            total_age += emp.age;
            count++;
        }
    }
    
    return count > 0 ? total_age / count : 0.0;
}

// Поиск максимальной зп у среднего возраста
double find_max_salary_near_average(const vector<Employee>& employees, 
                                   const string& target_position, 
                                   double average_age, 
                                   int age_range) {
    double max_salary = 0.0;
    
    for (const auto& emp : employees) {
        if (emp.position == target_position && 
            abs(emp.age - average_age) <= age_range) {
            if (emp.salary > max_salary) {
                max_salary = emp.salary;
            }
        }
    }
    
    return max_salary;
}

// Однопоточная обработка(просто применение всего)
void process_single_thread(const vector<Employee>& employees, 
                          const string& target_position) {
    // Расчет среднего возраста
    double average_age = calculate_average_age(employees, target_position);
    
    // Поиск максимальной зарплаты
    double max_salary = find_max_salary_near_average(employees, target_position, average_age);
    
    // Подсчет количества сотрудников с целевой должностью
    int target_count = 0;
    for (const auto& emp : employees) {
        if (emp.position == target_position) {
            target_count++;
        }
    }
    
    cout << "\n=== Результаты обработки (однопоток) ===\n";
    cout << "Всего сотрудников: " << employees.size() << "\n";
    cout << "Сотрудников с должностью '" << target_position << "': " << target_count << "\n\n";
    
    if (target_count > 0) {
        cout << "Средний возраст: " << fixed << setprecision(2) << average_age << " лет\n";
        cout << "Максимальная зарплата среди сотрудников\n";
        cout << "с возрастом +-2 года от среднего: " 
                  << fixed << setprecision(2) << max_salary << " руб.\n";
    } else {
        cout << "Нет сотрудников с должностью '" << target_position << "'\n";
    }
}

// Многопоток
void process_multi_thread(const vector<Employee>& employees, 
                         const string& target_position, 
                         int num_threads) {
    if (employees.empty()) {
        cout << "Нет данных для обработки\n";
        return;
    }
    
    // Генерация потоков
    vector<thread> threads;
    vector<double> thread_ages(num_threads, 0.0);
    vector<int> thread_counts(num_threads, 0);
    vector<double> thread_max_salaries(num_threads, 0.0);
    
    // Разбивка данных на чанки
    int chunk_size = employees.size() / num_threads;
    mutex cout_mutex;
    
    // Потоки параллельно работают с данными
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            int start = i * chunk_size;
            int end = (i == num_threads - 1) ? employees.size() : start + chunk_size;
            
            for (int j = start; j < end; ++j) {
                const auto& emp = employees[j];
                if (emp.position == target_position) {
                    thread_ages[i] += emp.age;
                    thread_counts[i]++;
                    
                    // Пока не знаем средний возраст(все потоки еще не отработали), сохраняем максимальную зарплату
                    if (emp.salary > thread_max_salaries[i]) {
                        thread_max_salaries[i] = emp.salary;
                    }
                }
            }
        });
    }
    // Синхроним джойном
    for (auto& t : threads) {
        t.join();
    }
    
    // Собрали результаты
    double total_age = 0.0;
    int total_count = 0;
    
    for (int i = 0; i < num_threads; ++i) {
        total_age += thread_ages[i];
        total_count += thread_counts[i];
    }
    
    double average_age = total_count > 0 ? total_age / total_count : 0.0;
    
    // Вторая фаза: поиск максимальной зарплаты с учетом среднего возраста(повторно проходимся по данным)

    threads.clear(); // Стирает потоки которые уже завершились
    vector<double> thread_phase2_max(num_threads, 0.0);
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i, average_age]() {
            int start = i * chunk_size;
            int end = (i == num_threads - 1) ? employees.size() : start + chunk_size;
            
            for (int j = start; j < end; ++j) {
                const auto& emp = employees[j];
                if (emp.position == target_position && 
                    abs(emp.age - average_age) <= 2) {
                    if (emp.salary > thread_phase2_max[i]) {
                        thread_phase2_max[i] = emp.salary;
                    }
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    double max_salary = 0.0;
    for (int i = 0; i < num_threads; ++i) {
        if (thread_phase2_max[i] > max_salary) {
            max_salary = thread_phase2_max[i];
        }
    }
    
    cout << "\n=== Результаты обработки (многопоток) ===\n";
    cout << "Использовано потоков: " << num_threads << "\n";
    cout << "Всего сотрудников: " << employees.size() << "\n";
    cout << "Сотрудников с должностью '" << target_position << "': " << total_count << "\n\n";
    
    if (total_count > 0) {
        cout << "Средний возраст: " << fixed << setprecision(2) << average_age << " лет\n";
        cout << "Максимальная зарплата среди сотрудников\n";
        cout << "с возрастом +-2 года от среднего: " 
                  << fixed << setprecision(2) << max_salary << " руб.\n";
    } else {
        cout << "Нет сотрудников с должностью '" << target_position << "'\n";
    }
}

// Анализ производительности
void analyze_performance(int min_size, int max_size, int step, 
                        const string& target_position) {
    cout << "\n=== Анализ производительности ===\n";
    cout << "Тест обработки разных объемов данных\n";
    cout << "Целевая должность: '" << target_position << "'\n\n";
    
    vector<pair<string, double>> single_thread_results;
    vector<pair<string, double>> multi_thread_results;
    
    for (int size = min_size; size <= max_size; size += step) {
        cout << "Тест с " << size << " сотрудниками...\n";
        
        auto employees = generate_employees(size, target_position);
        
        double single_time, multi_time;
        
        {
            Benchmark b("Однопоток", false);
            process_single_thread(employees, target_position);
            single_time = b.elapsed_microseconds();
        }
        
        {
            Benchmark b("Многопоток (4 потока)", false);
            process_multi_thread(employees, target_position, 4);
            multi_time = b.elapsed_microseconds();
        }
        
        single_thread_results.emplace_back(to_string(size), single_time);
        multi_thread_results.emplace_back(to_string(size), multi_time);
        
        double speedup = single_time / multi_time;
        cout << "  Ускорение: " << fixed << setprecision(2) << speedup << "x\n\n";
    }
    
    cout << "\n=== Итоги анализа производительности ===\n";
    cout << setw(10) << "Размер" 
              << setw(20) << "Однопоток (мс)"
              << setw(20) << "Многопоток (мс)"
              << setw(15) << "Ускорение" << "\n";
    cout << string(65, '-') << endl;
    
    for (size_t i = 0; i < single_thread_results.size(); ++i) {
        double single_ms = single_thread_results[i].second / 1000.0;
        double multi_ms = multi_thread_results[i].second / 1000.0;
        double speedup = single_thread_results[i].second / multi_thread_results[i].second;
        
        cout << setw(10) << single_thread_results[i].first
                  << setw(20) << fixed << setprecision(2) << single_ms
                  << setw(20) << fixed << setprecision(2) << multi_ms
                  << setw(15) << fixed << setprecision(2) << speedup << "x\n";
    }
    cout << string(65, '-') << endl;
}

// бенчмарк
void run_employees_benchmark() {
    cout << "\n=== Бенчмарк анализа сотрудников ===\n";
    
    string target_position = "Инженер";
    vector<int> test_sizes = {1000, 5000, 10000, 50000, 100000, 1000000, 10000000};
    vector<int> thread_counts = {1, 2, 4, 8};
    
    vector<pair<string, double>> benchmark_results;
    
    for (int size : test_sizes) {
        cout << "\nГенерация " << size << " сотрудников...\n";
        auto employees = generate_employees(size, target_position);
        
        for (int threads : thread_counts) {
            string test_name = to_string(size) + "_сотр_" + to_string(threads) + "_потоков";
            
            Benchmark b(test_name, false);
            if (threads == 1) {
                process_single_thread(employees, target_position);
            } else {
                process_multi_thread(employees, target_position, threads);
            }
            
            benchmark_results.emplace_back(test_name, b.elapsed_microseconds());
        }
    }
    
    Benchmark::save_to_csv(benchmark_results, "employees_benchmark.csv");
    cout << "\nБенчмарк завершен. Результаты сохранены в employees_benchmark.csv\n";
}

void run_employees() {
    cout << "\n=== Задание 2: Анализ сотрудников ===\n";
    cout << "Найти средний возраст для должности Д\n";
    cout << "и наибольшую зарплату среди сотрудников должности Д,\n";
    cout << "чья возраст отличается от среднего не более чем на 2 года.\n\n";
    
    int choice;
    cout << "Выберите режим:\n";
    cout << "1. Стандартный анализ\n";
    cout << "2. Анализ производительности\n";
    cout << "3. Полный бенчмарк\n";
    cout << "Ваш выбор: ";
    cin >> choice;
    
    string target_position;
    cout << "\nВведите целевую должность (Д): ";
    cin.ignore();
    getline(cin, target_position);
    // дефолтная должность инженер
    if (target_position.empty()) {
        target_position = "Инженер";
    }
    
    switch (choice) {
        case 1: {
            int num_employees, num_threads;
            
            cout << "\nВведите количество сотрудников (100-10000000): ";
            cin >> num_employees;
            
            if (num_employees < 100) num_employees = 100;
            if (num_employees > 10000000) num_employees = 10000000;
            
            cout << "Генерация " << num_employees << " сотрудников...\n";
            auto employees = generate_employees(num_employees, target_position);
            
            double single_time, multi_time;
            
            {
                Benchmark b("Однопоточная обработка");
                process_single_thread(employees, target_position);
                single_time = b.elapsed_microseconds();
            }
            
            cout << "\nВведите количество потоков для многопоточной обработки (2-16): ";
            cin >> num_threads;
            
            if (num_threads < 2) num_threads = 2;
            if (num_threads > 16) num_threads = 16;
            
            {
                Benchmark b("Многопоточная обработка");
                process_multi_thread(employees, target_position, num_threads);
                multi_time = b.elapsed_microseconds();
            }
            
            Benchmark::print_comparison("Однопоток", single_time, 
                                       "Многопоток (" + to_string(num_threads) + " потоков)", 
                                       multi_time);
            break;
        }
        case 2:
            analyze_performance(1000, 10000, 2000, target_position);
            break;
        case 3:
            run_employees_benchmark();
            break;
        default:
            cout << "Неверный выбор! Запускаю стандартный анализ...\n";
            auto employees = generate_employees(5000, target_position);
            process_single_thread(employees, target_position);
            process_multi_thread(employees, target_position, 4);
    }
}

} 
