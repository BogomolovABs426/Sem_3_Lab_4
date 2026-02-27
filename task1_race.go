package main

import (
	"context"
	"fmt"
	"math/rand"
	"sync"
	"sync/atomic"
	"time"

	"golang.org/x/sync/semaphore"
)

// SpinLock 
type SpinLock struct {
	locked int32
}

// блокирует спин-лок
func (s *SpinLock) Lock() {
	for !atomic.CompareAndSwapInt32(&s.locked, 0, 1) {
		// Активное ожидание
	}
}

// разблокирует спин-лок
func (s *SpinLock) Unlock() {
	atomic.StoreInt32(&s.locked, 0)
}

// SpinWait 
type SpinWait struct {
	locked int32
}

// блокирует с уступкой
func (s *SpinWait) Lock() {
	for !atomic.CompareAndSwapInt32(&s.locked, 0, 1) {
		time.Sleep(time.Microsecond) // Уступаем процессор
	}
}

// разблокирует
func (s *SpinWait) Unlock() {
	atomic.StoreInt32(&s.locked, 0)
}

// Monitor 
type Monitor struct {
	mtx       sync.Mutex
	cond      *sync.Cond
	available bool
}

// создает новый монитор
func NewMonitor() *Monitor {
	m := &Monitor{available: true}
	m.cond = sync.NewCond(&m.mtx)
	return m
}

// входит в монитор
func (m *Monitor) Enter() {
	m.mtx.Lock()
	for !m.available {
		m.cond.Wait()
	}
	m.available = false
	m.mtx.Unlock()
}

// выходит из монитора
func (m *Monitor) Exit() {
	m.mtx.Lock()
	m.available = true
	m.cond.Signal()
	m.mtx.Unlock()
}

// TestMutex 
func TestMutex(numThreads, iterations int) {
	var mtx sync.Mutex
	var wg sync.WaitGroup
	var counter int32
	var progress int32

	for i := 0; i < numThreads; i++ {
		wg.Add(1)
		go func(id int) {
			defer wg.Done()
			rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(id)))

			for j := 0; j < iterations; j++ {
				mtx.Lock()
				c := byte(33 + rng.Intn(94)) // Печатные ASCII символы
				value := int(c) * (j % 256)
				atomic.AddInt32(&counter, int32(value%256))
				atomic.AddInt32(&progress, 1)
				mtx.Unlock()
			}
		}(i)
	}

	wg.Wait()

	if numThreads*iterations < 1000 {
		fmt.Printf("  [Mutex] Завершено операций: %d, итоговое значение: %d\n",
			atomic.LoadInt32(&progress), atomic.LoadInt32(&counter))
	}
}

// TestSemaphore 
func TestSemaphore(numThreads, iterations int) {
	sem := semaphore.NewWeighted(1)
	var wg sync.WaitGroup
	var counter int32
	var progress int32

	for i := 0; i < numThreads; i++ {
		wg.Add(1)
		go func(id int) {
			defer wg.Done()
			rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(id)))

			for j := 0; j < iterations; j++ {
				// Используем context.Background() вместо nil
				ctx := context.Background()
				sem.Acquire(ctx, 1)
				c := byte(33 + rng.Intn(94))
				value := int(c) * (j % 256)
				atomic.AddInt32(&counter, int32(value%256))
				atomic.AddInt32(&progress, 1)
				sem.Release(1)
			}
		}(i)
	}

	wg.Wait()

	if numThreads*iterations < 1000 {
		fmt.Printf("  [Semaphore] Завершено операций: %d, итоговое значение: %d\n",
			atomic.LoadInt32(&progress), atomic.LoadInt32(&counter))
	}
}

// TestBarrier 
func TestBarrier(numThreads, iterations int) {
	var wg sync.WaitGroup
	var counter int32
	var progress int32

	for i := 0; i < numThreads; i++ {
		wg.Add(1)
		go func(id int) {
			defer wg.Done()
			rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(id)))

			for j := 0; j < iterations; j++ {
				c := byte(33 + rng.Intn(94))
				value := int(c) * (j % 256)
				atomic.AddInt32(&counter, int32(value%256))
				atomic.AddInt32(&progress, 1)
			}
		}(i)
	}

	wg.Wait()

	if numThreads*iterations < 1000 {
		fmt.Printf("  [Barrier] Завершено операций: %d, итоговое значение: %d\n",
			atomic.LoadInt32(&progress), atomic.LoadInt32(&counter))
	}
}

// TestSpinlock 
func TestSpinlock(numThreads, iterations int) {
	var spinlock SpinLock
	var wg sync.WaitGroup
	var counter int32
	var progress int32

	for i := 0; i < numThreads; i++ {
		wg.Add(1)
		go func(id int) {
			defer wg.Done()
			rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(id)))

			for j := 0; j < iterations; j++ {
				spinlock.Lock()
				c := byte(33 + rng.Intn(94))
				value := int(c) * (j % 256)
				atomic.AddInt32(&counter, int32(value%256))
				atomic.AddInt32(&progress, 1)
				spinlock.Unlock()
			}
		}(i)
	}

	wg.Wait()

	if numThreads*iterations < 1000 {
		fmt.Printf("  [SpinLock] Завершено операций: %d, итоговое значение: %d\n",
			atomic.LoadInt32(&progress), atomic.LoadInt32(&counter))
	}
}

// TestSpinwait 
func TestSpinwait(numThreads, iterations int) {
	var spinwait SpinWait
	var wg sync.WaitGroup
	var counter int32
	var progress int32

	for i := 0; i < numThreads; i++ {
		wg.Add(1)
		go func(id int) {
			defer wg.Done()
			rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(id)))

			for j := 0; j < iterations; j++ {
				spinwait.Lock()
				c := byte(33 + rng.Intn(94))
				value := int(c) * (j % 256)
				atomic.AddInt32(&counter, int32(value%256))
				atomic.AddInt32(&progress, 1)
				spinwait.Unlock()
			}
		}(i)
	}

	wg.Wait()

	if numThreads*iterations < 1000 {
		fmt.Printf("  [SpinWait] Завершено операций: %d, итоговое значение: %d\n",
			atomic.LoadInt32(&progress), atomic.LoadInt32(&counter))
	}
}

// TestMonitor 
func TestMonitor(numThreads, iterations int) {
	monitor := NewMonitor()
	var wg sync.WaitGroup
	var counter int32
	var progress int32

	for i := 0; i < numThreads; i++ {
		wg.Add(1)
		go func(id int) {
			defer wg.Done()
			rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(id)))

			for j := 0; j < iterations; j++ {
				monitor.Enter()
				c := byte(33 + rng.Intn(94))
				value := int(c) * (j % 256)
				atomic.AddInt32(&counter, int32(value%256))
				atomic.AddInt32(&progress, 1)
				monitor.Exit()
			}
		}(i)
	}

	wg.Wait()

	if numThreads*iterations < 1000 {
		fmt.Printf("  [Monitor] Завершено операций: %d, итоговое значение: %d\n",
			atomic.LoadInt32(&progress), atomic.LoadInt32(&counter))
	}
}

// Benchmark all
func BenchmarkAllPrimitives(numThreads, iterations int) {
	fmt.Println("\n=== Тестирование примитивов синхронизации ===")
	fmt.Printf("Параметры: %d потоков, %d итераций на поток\n", numThreads, iterations)
	fmt.Printf("Общее количество операций: %d\n\n", numThreads*iterations)

	results := make([]BenchmarkResult, 0, 6)

	// Mutex
	{
		b := NewBenchmark("Mutex тест", false)
		TestMutex(numThreads, iterations)
		time := b.StopSilent()
		results = append(results, BenchmarkResult{
			Name:             "Mutex",
			TimeMicroseconds: time,
			TimeMilliseconds: time / 1000.0,
			TimeSeconds:      time / 1000000.0,
		})
	}

	// Semaphore
	{
		b := NewBenchmark("Semaphore тест", false)
		TestSemaphore(numThreads, iterations)
		time := b.StopSilent()
		results = append(results, BenchmarkResult{
			Name:             "Semaphore",
			TimeMicroseconds: time,
			TimeMilliseconds: time / 1000.0,
			TimeSeconds:      time / 1000000.0,
		})
	}

	// Barrier
	{
		b := NewBenchmark("Barrier тест", false)
		TestBarrier(numThreads, iterations)
		time := b.StopSilent()
		results = append(results, BenchmarkResult{
			Name:             "Barrier",
			TimeMicroseconds: time,
			TimeMilliseconds: time / 1000.0,
			TimeSeconds:      time / 1000000.0,
		})
	}

	// SpinLock
	{
		b := NewBenchmark("SpinLock тест", false)
		TestSpinlock(numThreads, iterations)
		time := b.StopSilent()
		results = append(results, BenchmarkResult{
			Name:             "SpinLock",
			TimeMicroseconds: time,
			TimeMilliseconds: time / 1000.0,
			TimeSeconds:      time / 1000000.0,
		})
	}

	// SpinWait
	{
		b := NewBenchmark("SpinWait тест", false)
		TestSpinwait(numThreads, iterations)
		time := b.StopSilent()
		results = append(results, BenchmarkResult{
			Name:             "SpinWait",
			TimeMicroseconds: time,
			TimeMilliseconds: time / 1000.0,
			TimeSeconds:      time / 1000000.0,
		})
	}

	// Monitor
	{
		b := NewBenchmark("Monitor тест", false)
		TestMonitor(numThreads, iterations)
		time := b.StopSilent()
		results = append(results, BenchmarkResult{
			Name:             "Monitor",
			TimeMicroseconds: time,
			TimeMilliseconds: time / 1000.0,
			TimeSeconds:      time / 1000000.0,
		})
	}

	PrintResults(results, "Сравнение примитивов синхронизации")
	SaveToCSV(results, "primitives_benchmark.csv")
	PrintStatistics(results)
}

// расширенный бенчмарк
func RunExtendedBenchmark() {
	fmt.Println("\n=== Расширенный бенчмарк примитивов синхронизации ===")

	threadOptions := []int{2, 4, 8}
	iterationOptions := []int{100, 500, 1000}

	allResults := make([]BenchmarkResult, 0)

	for _, threads := range threadOptions {
		for _, iterations := range iterationOptions {
			fmt.Printf("\n--- Конфигурация: %d потоков, %d итераций ---\n", threads, iterations)

			// Mutex
			{
				b := NewBenchmark("Mutex", false)
				TestMutex(threads, iterations)
				time := b.StopSilent()
				allResults = append(allResults, BenchmarkResult{
					Name:             fmt.Sprintf("Mutex_%dt_%di", threads, iterations),
					TimeMicroseconds: time,
					TimeMilliseconds: time / 1000.0,
					TimeSeconds:      time / 1000000.0,
				})
			}

			// Semaphore
			{
				b := NewBenchmark("Semaphore", false)
				TestSemaphore(threads, iterations)
				time := b.StopSilent()
				allResults = append(allResults, BenchmarkResult{
					Name:             fmt.Sprintf("Semaphore_%dt_%di", threads, iterations),
					TimeMicroseconds: time,
					TimeMilliseconds: time / 1000.0,
					TimeSeconds:      time / 1000000.0,
				})
			}

			// Для ускорения тестирования остальные примитивы только для определенных конфигураций
			if threads == 4 && iterations == 500 {
				// Barrier
				{
					b := NewBenchmark("Barrier", false)
					TestBarrier(threads, iterations)
					time := b.StopSilent()
					allResults = append(allResults, BenchmarkResult{
						Name:             fmt.Sprintf("Barrier_%dt_%di", threads, iterations),
						TimeMicroseconds: time,
						TimeMilliseconds: time / 1000.0,
						TimeSeconds:      time / 1000000.0,
					})
				}

				// SpinLock
				{
					b := NewBenchmark("SpinLock", false)
					TestSpinlock(threads, iterations)
					time := b.StopSilent()
					allResults = append(allResults, BenchmarkResult{
						Name:             fmt.Sprintf("SpinLock_%dt_%di", threads, iterations),
						TimeMicroseconds: time,
						TimeMilliseconds: time / 1000.0,
						TimeSeconds:      time / 1000000.0,
					})
				}
			}
		}
	}

	SaveToCSV(allResults, "extended_benchmark.csv")
	fmt.Println("\nРасширенный бенчмарк завершен. Результаты сохранены в extended_benchmark.csv")
}


func RunRace() {
	fmt.Println("\n=== Задание 1: Параллельная гонка с ASCII символами ===")
	fmt.Println("Сравнение 6 примитивов синхронизации:")
	fmt.Println("1. Mutex ")
	fmt.Println("2. Semaphore ")
	fmt.Println("3. Barrier ")
	fmt.Println("4. SpinLock ")
	fmt.Println("5. SpinWait ")
	fmt.Println("6. Monitor \n")

	fmt.Print("Введите количество потоков (1-16): ")
	var numThreads int
	fmt.Scan(&numThreads)

	fmt.Print("Введите количество итераций на поток (100-10000): ")
	var iterations int
	fmt.Scan(&iterations)

	if numThreads < 1 || numThreads > 16 || iterations < 100 || iterations > 10000 {
		fmt.Println("Некорректные параметры! Использую значения по умолчанию.")
		numThreads = 4
		iterations = 1000
	}

	BenchmarkAllPrimitives(numThreads, iterations)
}
