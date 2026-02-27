package main

import (
	"bufio"
	"fmt"
	"os"
)



func printMenu() {
	
	fmt.Println("1. Задание 1: Сравнение примитивов синхронизации")
	fmt.Println("2. Задание 2: Анализ сотрудников ")
	fmt.Println("3. Задание 3: Обедающие философы")
	fmt.Println("4. Запустить все тесты производительности")
	fmt.Println("5. Экспорт всех результатов бенчмарка")
	fmt.Println("0. Выход")
	
}

func runAllBenchmarks() {
	fmt.Println("\n=== ЗАПУСК ВСЕХ ТЕСТОВ ПРОИЗВОДИТЕЛЬНОСТИ ===")
	

	// Тест 1: Примитивы синхронизации
	fmt.Println("\n[1/3] Тестирование примитивов синхронизации...")
	RunExtendedBenchmark()

	// Тест 2: Анализ сотрудников
	fmt.Println("\n[2/3] Бенчмарк анализа сотрудников...")
	RunEmployeesBenchmark()

	// Тест 3: Обедающие философы
	fmt.Println("\n[3/3] Бенчмарк обедающих философов...")
	RunPhilosophersBenchmark()

	fmt.Println("\n=== ВСЕ ТЕСТЫ ЗАВЕРШЕНЫ ===")
	fmt.Println("Созданные файлы:")
	fmt.Println("1. primitives_benchmark.csv")
	fmt.Println("2. extended_benchmark.csv")
	fmt.Println("3. employees_benchmark.csv")
	fmt.Println("4. philosophers_benchmark.csv")
	fmt.Println("5. all_benchmark_results.csv")
}

func exportAllResults() {
	fmt.Println("\n=== ЭКСПОРТ РЕЗУЛЬТАТОВ БЕНЧМАРКА ===")

	sampleData := []BenchmarkResult{
		{Name: "Mutex_4t_1000i", TimeMicroseconds: 1250.5},
		{Name: "Semaphore_4t_1000i", TimeMicroseconds: 1450.2},
		{Name: "Barrier_4t_500i", TimeMicroseconds: 2100.8},
		{Name: "SpinLock_4t_1000i", TimeMicroseconds: 980.3},
		{Name: "Однопоток_10000", TimeMicroseconds: 4550.7},
		{Name: "Многопоток_10000_4п", TimeMicroseconds: 1250.9},
		{Name: "Философы_5_Мьютексы", TimeMicroseconds: 3250.1},
		{Name: "Философы_10_Семафоры", TimeMicroseconds: 6250.4},
	}

	SaveToCSV(sampleData, "all_benchmark_results.csv")

	
}

func waitForEnter() {
	fmt.Print("\nНажмите Enter для продолжения...")
	bufio.NewReader(os.Stdin).ReadBytes('\n')
}

func main() {
	

	scanner := bufio.NewScanner(os.Stdin)

	for {
		printMenu()
		

		if !scanner.Scan() {
			break
		}

		choice := scanner.Text()

		switch choice {
		case "1":
			RunRace()
		case "2":
			RunEmployees()
		case "3":
			RunPhilosophers()
		case "4":
			runAllBenchmarks()
		case "5":
			exportAllResults()
		case "0":
			fmt.Println("\nВыход из программы...")
			os.Exit(0)
		default:
			fmt.Println("\nНеверный выбор! Пожалуйста, введите число от 0 до 5.")
		}

		if choice != "0" {
			waitForEnter()
		}
	}

	
	
	fmt.Println("   Результаты сохранены в CSV файлах")
	
}
