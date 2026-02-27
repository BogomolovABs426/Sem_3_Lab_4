package main

import (
	"fmt"
	"math/rand"
	"sync"
	"time"
)

// Employee представляет сотрудника
type Employee struct {
	Name     string  // ФИО
	Position string  // Должность
	Age      int     // Возраст
	Salary   float64 // Заработная плата
}

// GenerateEmployees генерирует список сотрудников
func GenerateEmployees(count int, targetPosition string) []Employee {
	employees := make([]Employee, 0, count)

	// Списки для генерации данных
	firstNames := []string{"Иван", "Петр", "Сергей", "Алексей", "Дмитрий",
		"Мария", "Ольга", "Елена", "Анна", "Наталья"}
	lastNames := []string{"Иванов", "Петров", "Сидоров", "Смирнов", "Кузнецов",
		"Попов", "Васильев", "Павлов", "Семенов", "Федоров"}
	middleNames := []string{"Иванович", "Петрович", "Сергеевич", "Алексеевич",
		"Дмитриевич", "Ивановна", "Петровна", "Сергеевна", "Алексеевна", "Дмитриевна"}

	positions := []string{"Менеджер", "Разработчик", "Аналитик", "Тестировщик",
		"Дизайнер", "Администратор", "Бухгалтер", targetPosition}

	rng := rand.New(rand.NewSource(time.Now().UnixNano()))

	for i := 0; i < count; i++ {
		// Генерация ФИО
		name := lastNames[rng.Intn(len(lastNames))] + " " +
			firstNames[rng.Intn(len(firstNames))] + " " +
			middleNames[rng.Intn(len(middleNames))]

		// Генерация должности
		position := positions[rng.Intn(len(positions))]

		// Генерация возраста (20-65 лет)
		age := 20 + rng.Intn(46)

		// Генерация зарплаты (30,000 - 300,000 руб.)
		salary := 30000 + rng.Float64()*270000

		employees = append(employees, Employee{
			Name:     name,
			Position: position,
			Age:      age,
			Salary:   salary,
		})
	}

	// Убедимся, что есть сотрудники с целевой должностью
	hasTargetPosition := false
	for _, emp := range employees {
		if emp.Position == targetPosition {
			hasTargetPosition = true
			break
		}
	}

	if !hasTargetPosition && len(employees) > 0 {
		employees[0].Position = targetPosition
	}

	return employees
}

// вычисляет средний возраст для указанной должности
func CalculateAverageAge(employees []Employee, targetPosition string) float64 {
	var totalAge int
	var count int

	for _, emp := range employees {
		if emp.Position == targetPosition {
			totalAge += emp.Age
			count++
		}
	}

	if count == 0 {
		return 0
	}
	return float64(totalAge) / float64(count)
}

// находит максимальную зарплату среди сотрудников,
// чей возраст отличается от среднего не более чем на ageRange лет
func FindMaxSalaryNearAverage(employees []Employee, targetPosition string,
	averageAge float64, ageRange int) float64 {
	maxSalary := 0.0

	for _, emp := range employees {
		if emp.Position == targetPosition {
			ageDiff := float64(emp.Age) - averageAge
			if ageDiff < 0 {
				ageDiff = -ageDiff
			}
			if ageDiff <= float64(ageRange) {
				if emp.Salary > maxSalary {
					maxSalary = emp.Salary
				}
			}
		}
	}

	return maxSalary
}

// обрабатывает сотрудников в одном потоке
func ProcessSingleThread(employees []Employee, targetPosition string) {
	// Расчет среднего возраста
	averageAge := CalculateAverageAge(employees, targetPosition)

	// Поиск максимальной зарплаты
	maxSalary := FindMaxSalaryNearAverage(employees, targetPosition, averageAge, 2)

	// Подсчет количества сотрудников с целевой должности
	targetCount := 0
	for _, emp := range employees {
		if emp.Position == targetPosition {
			targetCount++
		}
	}

	fmt.Println("\n=== Результаты обработки (однопоток) ===")
	fmt.Printf("Всего сотрудников: %d\n", len(employees))
	fmt.Printf("Сотрудников с должностью '%s': %d\n\n", targetPosition, targetCount)

	if targetCount > 0 {
		fmt.Printf("Средний возраст: %.2f лет\n", averageAge)
		fmt.Println("Максимальная зарплата среди сотрудников")
		fmt.Printf("с возрастом +-2 года от среднего: %.2f руб.\n", maxSalary)
	} else {
		fmt.Printf("Нет сотрудников с должностью '%s'\n", targetPosition)
	}
}

// обрабатывает сотрудников в нескольких потоках
func ProcessMultiThread(employees []Employee, targetPosition string, numThreads int) {
	if len(employees) == 0 {
		fmt.Println("Нет данных для обработки")
		return
	}

	var wg sync.WaitGroup
	threadAges := make([]float64, numThreads)
	threadCounts := make([]int, numThreads)
	threadMaxSalaries := make([]float64, numThreads)

	chunkSize := len(employees) / numThreads

	// Первая фаза: вычисление среднего возраста
	for i := 0; i < numThreads; i++ {
		wg.Add(1)
		go func(threadID int) {
			defer wg.Done()

			start := threadID * chunkSize
			end := start + chunkSize
			if threadID == numThreads-1 {
				end = len(employees)
			}

			for j := start; j < end; j++ {
				emp := employees[j]
				if emp.Position == targetPosition {
					threadAges[threadID] += float64(emp.Age)
					threadCounts[threadID]++

					// Сохраняем максимальную зарплату (пока не знаем средний возраст)
					if emp.Salary > threadMaxSalaries[threadID] {
						threadMaxSalaries[threadID] = emp.Salary
					}
				}
			}
		}(i)
	}

	wg.Wait()

	// Агрегируем результаты для расчета среднего возраста
	var totalAge float64
	var totalCount int
	for i := 0; i < numThreads; i++ {
		totalAge += threadAges[i]
		totalCount += threadCounts[i]
	}

	averageAge := 0.0
	if totalCount > 0 {
		averageAge = totalAge / float64(totalCount)
	}

	// Вторая фаза: поиск максимальной зарплаты с учетом среднего возраста
	wg = sync.WaitGroup{}
	threadPhase2Max := make([]float64, numThreads)

	for i := 0; i < numThreads; i++ {
		wg.Add(1)
		go func(threadID int, avgAge float64) {
			defer wg.Done()

			start := threadID * chunkSize
			end := start + chunkSize
			if threadID == numThreads-1 {
				end = len(employees)
			}

			for j := start; j < end; j++ {
				emp := employees[j]
				if emp.Position == targetPosition {
					ageDiff := float64(emp.Age) - avgAge
					if ageDiff < 0 {
						ageDiff = -ageDiff
					}
					if ageDiff <= 2 {
						if emp.Salary > threadPhase2Max[threadID] {
							threadPhase2Max[threadID] = emp.Salary
						}
					}
				}
			}
		}(i, averageAge)
	}

	wg.Wait()

	// Находим общую максимальную зарплату
	maxSalary := 0.0
	for i := 0; i < numThreads; i++ {
		if threadPhase2Max[i] > maxSalary {
			maxSalary = threadPhase2Max[i]
		}
	}

	fmt.Println("\n=== Результаты обработки (многопоток) ===")
	fmt.Printf("Использовано потоков: %d\n", numThreads)
	fmt.Printf("Всего сотрудников: %d\n", len(employees))
	fmt.Printf("Сотрудников с должностью '%s': %d\n\n", targetPosition, totalCount)

	if totalCount > 0 {
		fmt.Printf("Средний возраст: %.2f лет\n", averageAge)
		fmt.Println("Максимальная зарплата среди сотрудников")
		fmt.Printf("с возрастом +-2 года от среднего: %.2f руб.\n", maxSalary)
	} else {
		fmt.Printf("Нет сотрудников с должностью '%s'\n", targetPosition)
	}
}


func RunEmployees() {
	fmt.Println("\n=== Задание 2: Анализ сотрудников ===")
	fmt.Println("Найти средний возраст для должности Д")
	fmt.Println("и наибольшую зарплату среди сотрудников должности Д,")
	fmt.Println("чья возраст отличается от среднего не более чем на 2 года.\n")

	fmt.Print("Введите целевую должность (Д): ")
	var targetPosition string
	fmt.Scanln()
	fmt.Scanln(&targetPosition)

	if targetPosition == "" {
		targetPosition = "Инженер"
	}

	var choice string
	fmt.Println("\nВыберите режим:")
	fmt.Println("1. Стандартный анализ")
	fmt.Println("2. Анализ производительности")
	fmt.Println("3. Полный бенчмарк")
	fmt.Print("Ваш выбор: ")
	fmt.Scan(&choice)

	switch choice {
	case "1":
		var numEmployees, numThreads int

		fmt.Print("\nВведите количество сотрудников (100-100000): ")
		fmt.Scan(&numEmployees)

		if numEmployees < 100 {
			numEmployees = 100
		}
		if numEmployees > 100000 {
			numEmployees = 100000
		}

		fmt.Printf("Генерация %d сотрудников...\n", numEmployees)
		employees := GenerateEmployees(numEmployees, targetPosition)

		var singleTime, multiTime float64

		// Однопоточная обработка
		{
			b := NewBenchmark("Однопоточная обработка")
			ProcessSingleThread(employees, targetPosition)
			singleTime = b.StopSilent()
		}

		fmt.Print("\nВведите количество потоков для многопоточной обработки (2-16): ")
		fmt.Scan(&numThreads)

		if numThreads < 2 {
			numThreads = 2
		}
		if numThreads > 16 {
			numThreads = 16
		}

		// Многопоточная обработка
		{
			b := NewBenchmark("Многопоточная обработка")
			ProcessMultiThread(employees, targetPosition, numThreads)
			multiTime = b.StopSilent()
		}

		PrintComparison("Однопоток", singleTime,
			fmt.Sprintf("Многопоток (%d потоков)", numThreads), multiTime)

	case "2":
		AnalyzePerformance(1000, 10000, 2000, targetPosition)

	case "3":
		RunEmployeesBenchmark()

	default:
		fmt.Println("Неверный выбор! Запускаю стандартный анализ...")
		employees := GenerateEmployees(5000, targetPosition)
		ProcessSingleThread(employees, targetPosition)
		ProcessMultiThread(employees, targetPosition, 4)
	}
}

// бенчмарк
func RunEmployeesBenchmark() {
	fmt.Println("\n=== Бенчмарк анализа сотрудников ===")

	fmt.Print("Введите целевую должность (по умолчанию 'Инженер'): ")
	var targetPosition string
	fmt.Scanln()
	fmt.Scanln(&targetPosition)
	if targetPosition == "" {
		targetPosition = "Инженер"
	}

	testSizes := []int{1000, 5000, 10000, 50000, 100000}
	threadCounts := []int{1, 2, 4, 8}

	benchmarkResults := make([]BenchmarkResult, 0)

	for _, size := range testSizes {
		fmt.Printf("\nГенерация %d сотрудников...\n", size)
		employees := GenerateEmployees(size, targetPosition)

		for _, threads := range threadCounts {
			testName := fmt.Sprintf("%d_сотр_%d_потоков_%s", size, threads, targetPosition)

			b := NewBenchmark(testName, false)

			if threads == 1 {
				ProcessSingleThread(employees, targetPosition)
			} else {
				ProcessMultiThread(employees, targetPosition, threads)
			}

			time := b.StopSilent()
			benchmarkResults = append(benchmarkResults, BenchmarkResult{
				Name:             testName,
				TimeMicroseconds: time,
				TimeMilliseconds: time / 1000.0,
				TimeSeconds:      time / 1000000.0,
			})
		}
	}

	SaveToCSV(benchmarkResults, "employees_benchmark.csv")
	fmt.Println("\nБенчмарк завершен. Результаты сохранены в employees_benchmark.csv")
}

// анализ производительности
func AnalyzePerformance(minSize, maxSize, step int, targetPosition string) {
	fmt.Println("\n=== Анализ производительности ===")
	fmt.Printf("Целевая должность: '%s'\n\n", targetPosition)

	singleThreadResults := make([]BenchmarkResult, 0)
	multiThreadResults := make([]BenchmarkResult, 0)

	for size := minSize; size <= maxSize; size += step {
		fmt.Printf("Тест с %d сотрудниками...\n", size)

		employees := GenerateEmployees(size, targetPosition)

		var singleTime, multiTime float64

		// Однопоточная обработка
		{
			b := NewBenchmark("Однопоток", false)
			ProcessSingleThread(employees, targetPosition)
			singleTime = b.StopSilent()
		}

		// Многопоточная обработка (4 потока)
		{
			b := NewBenchmark("Многопоток (4 потока)", false)
			ProcessMultiThread(employees, targetPosition, 4)
			multiTime = b.StopSilent()
		}

		singleThreadResults = append(singleThreadResults, BenchmarkResult{
			Name:             fmt.Sprintf("%d", size),
			TimeMicroseconds: singleTime,
			TimeMilliseconds: singleTime / 1000.0,
			TimeSeconds:      singleTime / 1000000.0,
		})

		multiThreadResults = append(multiThreadResults, BenchmarkResult{
			Name:             fmt.Sprintf("%d", size),
			TimeMicroseconds: multiTime,
			TimeMilliseconds: multiTime / 1000.0,
			TimeSeconds:      multiTime / 1000000.0,
		})

		speedup := singleTime / multiTime
		fmt.Printf("  Ускорение: %.2fx\n\n", speedup)
	}

	fmt.Println("\n=== Итоги анализа производительности ===")
	fmt.Printf("%-10s %-20s %-20s %-15s\n", "Размер", "Однопоток (мс)", "Многопоток (мс)", "Ускорение")
	fmt.Println("-----------------------------------------------------------------")

	for i := 0; i < len(singleThreadResults); i++ {
		singleMS := singleThreadResults[i].TimeMilliseconds
		multiMS := multiThreadResults[i].TimeMilliseconds
		speedup := singleThreadResults[i].TimeMicroseconds / multiThreadResults[i].TimeMicroseconds

		fmt.Printf("%-10s %-20.2f %-20.2f %-15.2fx\n",
			singleThreadResults[i].Name, singleMS, multiMS, speedup)
	}
	fmt.Println("-----------------------------------------------------------------")
}
