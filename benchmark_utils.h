#ifndef BENCHMARK_UTILS_H
#define BENCHMARK_UTILS_H

#include <chrono>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <cmath>

using namespace std;

// класс бенчмарка
class Benchmark {
private:
    chrono::high_resolution_clock::time_point start_time;
    string benchmark_name;
    bool verbose;
    
public:
    Benchmark(const string& name, bool verbose_mode = true) 
        : benchmark_name(name), verbose(verbose_mode) {
        start_time = chrono::high_resolution_clock::now();
    }
    
    ~Benchmark() {
        if (verbose) {
            auto end_time = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(end_time - start_time);
            cout << "[" << benchmark_name << "] Время выполнения: " 
                      << duration.count() << " мкс (" 
                      << duration.count() / 1000.0 << " мс)" << std::endl;
        }
    }
    // для микромекунд
    double elapsed_microseconds() const {
        auto current_time = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(current_time - start_time);
        return static_cast<double>(duration.count());
    }
    // для миллисекунд
    double elapsed_milliseconds() const {
        return elapsed_microseconds() / 1000.0;
    }
    // для секунд
    double elapsed_seconds() const {
        return elapsed_microseconds() / 1000000.0;
    }
    
    // вывод результатов
    static void print_results(const vector<pair<string, double>>& results, 
                             const string& title = "Результаты бенчмарка") {
        cout << "\n=== " << title << " ===\n";
        cout << setw(20) << left << "Тест" 
                  << setw(15) << "Время (мкс)"
                  << setw(15) << "Время (мс)" << "\n";
        cout << string(50, '-') << endl;
        
        for (const auto& result : results) {
            cout << setw(20) << left << result.first 
                      << setw(15) << fixed << setprecision(2) << result.second
                      << setw(15) << fixed << setprecision(4) << result.second / 1000.0 
                      << endl;
        }
        cout << string(50, '=') << "\n" << endl;
    }
    // вывод сравнения
    static void print_comparison(const string& test1_name, double time1,
                                const string& test2_name, double time2) {
        cout << "\n=== Сравнение производительности ===\n";
        cout << setw(25) << left << "Метод"
                  << setw(15) << "Время (мс)"
                  << setw(15) << "Ускорение" << "\n";
        cout << string(55, '-') << endl;
        
        double speedup = time1 / time2;
        cout << setw(25) << left << test1_name
                  << setw(15) << fixed << setprecision(3) << time1 / 1000.0
                  << setw(15) << "1.00x" << endl;
        
        cout << setw(25) << left << test2_name
                  << setw(15) << fixed << setprecision(3) << time2 / 1000.0
                  << setw(15) << fixed << setprecision(2) << speedup << "x" << endl;
        cout << string(55, '-') << endl;
    }
    // работа с csv файлами
    static void save_to_csv(const vector<pair<string, double>>& results,
                           const string& filename = "benchmark_results.csv") {
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Ошибка: не удалось создать файл " << filename << endl;
            return;
        }
        
        file << "Тест,Время(микросекунды),Время(миллисекунды),Время(секунды)\n";
        
        for (const auto& result : results) {
            file << result.first << ","
                 << result.second << ","
                 << result.second / 1000.0 << ","
                 << result.second / 1000000.0 << "\n";
        }
        
        file.close();
        cout << "Результаты сохранены в файл: " << filename << endl;
    }
    // вывод статистики
    static void print_statistics(const vector<pair<string, double>>& results) {
        if (results.empty()) return;
        
        double min_time = results[0].second;
        double max_time = results[0].second;
        double sum = 0;
        string fastest = results[0].first;
        string slowest = results[0].first;
        
        for (const auto& result : results) {
            double time = result.second;
            sum += time;
            
            if (time < min_time) {
                min_time = time;
                fastest = result.first;
            }
            
            if (time > max_time) {
                max_time = time;
                slowest = result.first;
            }
        }
        
        double avg = sum / results.size();
        
        // стандартное отклонение
        double variance = 0;
        for (const auto& result : results) {
            variance += pow(result.second - avg, 2);
        }
        variance /= results.size();
        double stddev = sqrt(variance);
        
        cout << "\n=== Статистика бенчмарка ===\n";
        cout << "Количество тестов: " << results.size() << "\n";
        cout << "Среднее время: " << avg << " мкс (" << avg/1000.0 << " мс)\n";
        cout << "Минимальное время: " << min_time << " мкс (" << fastest << ")\n";
        cout << "Максимальное время: " << max_time << " мкс (" << slowest << ")\n";
        cout << "Стандартное отклонение: " << stddev << " мкс\n";
        cout << "Разброс: " << (max_time - min_time) << " мкс ("
                  << fixed << setprecision(1) 
                  << ((max_time - min_time) / min_time * 100) << "%)\n";
    }
};

#endif 
