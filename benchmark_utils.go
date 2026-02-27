package main

import (
	"encoding/csv"
	"fmt"
	"math"
	"os"
	"time"
)

// BenchmarkResult представляет результат бенчмарка
type BenchmarkResult struct {
	Name             string
	TimeMicroseconds float64
	TimeMilliseconds float64
	TimeSeconds      float64
}

// Benchmark измеряет время выполнения
type Benchmark struct {
	start   time.Time
	name    string
	verbose bool
}

// NewBenchmark создает новый бенчмарк
func NewBenchmark(name string, verbose ...bool) *Benchmark {
	v := true
	if len(verbose) > 0 {
		v = verbose[0]
	}

	return &Benchmark{
		start:   time.Now(),
		name:    name,
		verbose: v,
	}
}

// Stop останавливает бенчмарк и выводит результаты
func (b *Benchmark) Stop() float64 {
	elapsed := time.Since(b.start)
	microseconds := float64(elapsed.Microseconds())

	if b.verbose {
		fmt.Printf("[%s] Время выполнения: %.2f мкс (%.4f мс)\n",
			b.name, microseconds, microseconds/1000.0)
	}

	return microseconds
}

// StopSilent останавливает бенчмарк без вывода
func (b *Benchmark) StopSilent() float64 {
	elapsed := time.Since(b.start)
	return float64(elapsed.Microseconds())
}

// PrintResults выводит результаты бенчмарка в табличном формате
func PrintResults(results []BenchmarkResult, title string) {
	fmt.Printf("\n=== %s ===\n", title)
	fmt.Printf("%-30s %-15s %-15s\n",
		"Тест", "Время (мкс)", "Время (мс)")
	fmt.Println("------------------------------------------------------------")

	for _, result := range results {
		fmt.Printf("%-30s %-15.2f %-15.4f\n",
			result.Name,
			result.TimeMicroseconds,
			result.TimeMilliseconds)
	}
	fmt.Println("============================================================\n")
}

// PrintComparison выводит сравнение двух тестов
func PrintComparison(test1Name string, time1 float64, test2Name string, time2 float64) {
	fmt.Println("\n=== Сравнение производительности ===")
	fmt.Printf("%-25s %-15s %-15s\n", "Метод", "Время (мс)", "Ускорение")
	fmt.Println("--------------------------------------------------")

	speedup := time1 / time2
	fmt.Printf("%-25s %-15.3f %-15s\n", test1Name, time1/1000.0, "1.00x")
	fmt.Printf("%-25s %-15.3f %-15.2fx\n", test2Name, time2/1000.0, speedup)
	fmt.Println("--------------------------------------------------")
}

// PrintStatistics выводит статистику результатов
func PrintStatistics(results []BenchmarkResult) {
	if len(results) == 0 {
		return
	}

	var sum, min, max float64
	min = results[0].TimeMicroseconds
	max = results[0].TimeMicroseconds
	fastest := results[0].Name
	slowest := results[0].Name

	for _, result := range results {
		time := result.TimeMicroseconds
		sum += time

		if time < min {
			min = time
			fastest = result.Name
		}

		if time > max {
			max = time
			slowest = result.Name
		}
	}

	average := sum / float64(len(results))

	// Вычисляем стандартное отклонение
	var variance float64
	for _, result := range results {
		diff := result.TimeMicroseconds - average
		variance += diff * diff
	}
	variance /= float64(len(results))
	stddev := math.Sqrt(variance)

	fmt.Println("\n=== Статистика бенчмарка ===")
	fmt.Printf("Количество тестов: %d\n", len(results))
	fmt.Printf("Среднее время: %.2f мкс (%.4f мс)\n", average, average/1000.0)
	fmt.Printf("Минимальное время: %.2f мкс (%s)\n", min, fastest)
	fmt.Printf("Максимальное время: %.2f мкс (%s)\n", max, slowest)
	fmt.Printf("Стандартное отклонение: %.2f мкс\n", stddev)
	fmt.Printf("Разброс: %.2f мкс (%.1f%%)\n",
		max-min, ((max-min)/min)*100)
}

// SaveToCSV сохраняет результаты в CSV файл
func SaveToCSV(results []BenchmarkResult, filename string) error {
	file, err := os.Create(filename)
	if err != nil {
		return fmt.Errorf("не удалось создать файл %s: %v", filename, err)
	}
	defer file.Close()

	writer := csv.NewWriter(file)
	defer writer.Flush()

	// Заголовок
	writer.Write([]string{"Тест", "Время(микросекунды)", "Время(миллисекунды)", "Время(секунды)"})

	for _, result := range results {
		record := []string{
			result.Name,
			fmt.Sprintf("%.2f", result.TimeMicroseconds),
			fmt.Sprintf("%.4f", result.TimeMilliseconds),
			fmt.Sprintf("%.6f", result.TimeSeconds),
		}
		writer.Write(record)
	}

	fmt.Printf("Результаты сохранены в файл: %s\n", filename)
	return nil
}
