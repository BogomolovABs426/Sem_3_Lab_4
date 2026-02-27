#include <iostream>
#include "task1_race.h"
#include "task2_employees.h"
#include "task3_philosophers.h"
#include "benchmark_utils.h"  

using namespace std;

void print_menu() {
    cout << "1. Задание 1: Сравнение примитивов синхронизации\n";
    cout << "2. Задание 2: Анализ сотрудников\n";
    cout << "3. Задание 3: Обедающие философы\n";
    cout << "4. Запустить все тесты производительности\n";
    cout << "5. Экспорт всех результатов бенчмарка\n";
    cout << "0. Выход\n";
}

void run_all_benchmarks() {
    cout << "\n=== ЗАПУСК ВСЕХ ТЕСТОВ ПРОИЗВОДИТЕЛЬНОСТИ ===\n";
    cout << "Это может занять несколько минут...\n\n";
    
    // Тест 1: Примитивы синхронизации
    {
        cout << "\n[1/3] Тестирование примитивов синхронизации...\n";
        task1::run_extended_benchmark();
    }
    
    // Тест 2: Анализ сотрудников
    {
        cout << "\n[2/3] Бенчмарк анализа сотрудников...\n";
        task2::run_employees_benchmark();
    }
    
    // Тест 3: Обедающие философы
    {
        cout << "\n[3/3] Бенчмарк обедающих философов...\n";
        task3::run_philosophers_benchmark();
    }
    
    cout << "\n=== ВСЕ ТЕСТЫ ЗАВЕРШЕНЫ ===\n";
    cout << "Созданные файлы:\n";
    cout << "1. primitives_benchmark.csv\n";
    cout << "2. extended_benchmark.csv\n";
    cout << "3. employees_benchmark.csv\n";
    cout << "4. philosophers_benchmark.csv\n\n";
}

void export_all_results() {
    cout << "\n=== ЭКСПОРТ РЕЗУЛЬТАТОВ БЕНЧМАРКА ===\n";
    cout << "Генерация тестовых данных...\n\n";
    
    vector<pair<string, double>> sample_data = {
        {"Mutex_4t_1000i", 1250.5},
        {"Semaphore_4t_1000i", 1450.2},
        {"Barrier_4t_500i", 2100.8},
        {"SpinLock_4t_1000i", 980.3},
        {"Однопоточная_10000", 4550.7},
        {"Многопоточная_10000_4п", 1250.9},
        {"Философы_5", 3250.1},
        {"Философы_10", 6250.4}
    };
    
    Benchmark::save_to_csv(sample_data, "all_benchmark_results.csv");
} 


int main() {
    
    int choice;
    
    do {
        print_menu();

        cin >> choice;
        
        switch (choice) {
            case 1:
                task1::run_race();
                break;
            case 2:
                task2::run_employees();
                break;
            case 3:
                task3::run_philosophers();
                break;
            case 4:
                run_all_benchmarks();
                break;
            case 5:
                export_all_results();
                break;
            case 0:
                cout << "\nВыход из программы...\n";
                break;
            default:
                cout << "\nНеверный выбор! Пожалуйста, введите число от 0 до 5.\n";
        }
        
        if (choice != 0) {
            cout << "\nНажмите Enter для продолжения...";
            cin.ignore();
            cin.get();
        }
        
    } while (choice != 0);
    
    cout << "   Результаты сохранены в CSV файлах\n";
    
    return 0;
}
