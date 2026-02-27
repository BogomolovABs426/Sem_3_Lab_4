#ifndef TASK2_EMPLOYEES_H
#define TASK2_EMPLOYEES_H

#include <string>
#include <vector>
#include <chrono>

using namespace std;

namespace task2 {

struct Employee {
    string name;           // ФИО
    string position;       // Должность
    int age;               // Возраст
    double salary;         // Заработная плата
    
    Employee(string n = "", string p = "", int a = 0, double s = 0.0)
        : name(n), position(p), age(a), salary(s) {}
};

// Основные функции
void run_employees();
void run_employees_benchmark();

// Вспомогательные функции
vector<Employee> generate_employees(int count, const string& target_position);
double calculate_average_age(const vector<Employee>& employees, const string& target_position);
double find_max_salary_near_average(const vector<Employee>& employees, 
                                   const string& target_position, 
                                   double average_age, 
                                   int age_range = 2);

// Функции обработки
void process_single_thread(const vector<Employee>& employees, 
                          const string& target_position);
void process_multi_thread(const vector<Employee>& employees, 
                         const string& target_position, 
                         int num_threads);

// Анализ производительности
void analyze_performance(int min_size, int max_size, int step, 
                        const string& target_position);

} 

#endif 
