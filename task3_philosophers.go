package main

import (
	"context"
	"fmt"
	"math/rand"
	"sync"
	"time"

	"golang.org/x/sync/semaphore"
)

// определяет стратегию синхронизации
type Strategy int

const (
	MutexStrategy Strategy = iota
	SemaphoreStrategy
	TryLockStrategy
	ArbitratorStrategy
	ResourceHierarchyStrategy
)

// реализует задачу обедающих философов
type DiningPhilosophers struct {
	numPhilosophers int
	strategy        Strategy
}

// создает новую задачу философов
func NewDiningPhilosophers(numPhilosophers int, strategy Strategy) *DiningPhilosophers {
	return &DiningPhilosophers{
		numPhilosophers: numPhilosophers,
		strategy:        strategy,
	}
}

// реализует философа с использованием мьютексов
func (dp *DiningPhilosophers) philosopherMutex(id, iterations int, verbose bool, forks []*sync.Mutex, wg *sync.WaitGroup) {
	defer wg.Done()

	rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(id)))

	leftFork := id
	rightFork := (id + 1) % dp.numPhilosophers

	for i := 0; i < iterations; i++ {
		// Размышление
		thinkTime := 50 + rng.Intn(151) // 50-200 мс
		time.Sleep(time.Millisecond * time.Duration(thinkTime))

		if verbose && i < 10 {
			fmt.Printf("Философ %d размышляет (итерация %d)\n", id, i+1)
		}

		// Захват вилок в определенном порядке для избежания deadlock
		if id%2 == 0 {
			forks[leftFork].Lock()
			forks[rightFork].Lock()
		} else {
			forks[rightFork].Lock()
			forks[leftFork].Lock()
		}

		// Еда
		eatTime := 100 + rng.Intn(201) 
		time.Sleep(time.Millisecond * time.Duration(eatTime))

		if verbose && i < 10 {
			fmt.Printf("Философ %d ест спагетти (итерация %d)\n", id, i+1)
		}

		// Освобождение вилок
		forks[leftFork].Unlock()
		forks[rightFork].Unlock()
	}
}

// реализует философа с использованием семафоров
func (dp *DiningPhilosophers) philosopherSemaphore(id, iterations int, verbose bool, forks []*semaphore.Weighted, wg *sync.WaitGroup) {
	defer wg.Done()

	rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(id)))

	leftFork := id
	rightFork := (id + 1) % dp.numPhilosophers
	ctx := context.Background()

	for i := 0; i < iterations; i++ {
		// Размышление
		thinkTime := 50 + rng.Intn(151)
		time.Sleep(time.Millisecond * time.Duration(thinkTime))

		if verbose && i < 10 {
			fmt.Printf("Философ %d размышляет (итерация %d)\n", id, i+1)
		}

		// Захват вилок
		forks[leftFork].Acquire(ctx, 1)
		forks[rightFork].Acquire(ctx, 1)

		// Еда
		eatTime := 100 + rng.Intn(201)
		time.Sleep(time.Millisecond * time.Duration(eatTime))

		if verbose && i < 10 {
			fmt.Printf("Философ %d ест спагетти (итерация %d)\n", id, i+1)
		}

		// Освобождение вилок
		forks[leftFork].Release(1)
		forks[rightFork].Release(1)
	}
}

// реализует философа с попыткой захвата
func (dp *DiningPhilosophers) philosopherTryLock(id, iterations int, verbose bool, forks []*sync.Mutex, wg *sync.WaitGroup) {
	defer wg.Done()

	rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(id)))

	leftFork := id
	rightFork := (id + 1) % dp.numPhilosophers

	for i := 0; i < iterations; i++ {
		// Размышление
		thinkTime := 50 + rng.Intn(151)
		time.Sleep(time.Millisecond * time.Duration(thinkTime))

		if verbose && i < 10 {
			fmt.Printf("Философ %d размышляет (итерация %d)\n", id, i+1)
		}

		// Попытка захвата вилок с повторными попытками
		hasForks := false
		for !hasForks {
			if forks[leftFork].TryLock() {
				if forks[rightFork].TryLock() {
					hasForks = true
				} else {
					forks[leftFork].Unlock()
					retryTime := 10 + rng.Intn(41) 
					time.Sleep(time.Millisecond * time.Duration(retryTime))
				}
			} else {
				retryTime := 10 + rng.Intn(41)
				time.Sleep(time.Millisecond * time.Duration(retryTime))
			}
		}

		// Еда
		eatTime := 100 + rng.Intn(201)
		time.Sleep(time.Millisecond * time.Duration(eatTime))

		if verbose && i < 10 {
			fmt.Printf("Философ %d ест спагетти (итерация %d)\n", id, i+1)
		}

		// Освобождение вилок
		forks[leftFork].Unlock()
		forks[rightFork].Unlock()
	}
}

// реализует философа с арбитром
func (dp *DiningPhilosophers) philosopherArbitrator(id, iterations int, verbose bool,
	tableMutex *sync.Mutex, forksAvailable []bool, wg *sync.WaitGroup) {
	defer wg.Done()

	rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(id)))

	leftFork := id
	rightFork := (id + 1) % dp.numPhilosophers

	for i := 0; i < iterations; i++ {
		// Размышление
		thinkTime := 50 + rng.Intn(151)
		time.Sleep(time.Millisecond * time.Duration(thinkTime))

		if verbose && i < 10 {
			fmt.Printf("Философ %d размышляет (итерация %d)\n", id, i+1)
		}

		// Запрос разрешения у арбитра
		hasPermission := false
		for !hasPermission {
			tableMutex.Lock()

			if forksAvailable[leftFork] && forksAvailable[rightFork] {
				forksAvailable[leftFork] = false
				forksAvailable[rightFork] = false
				hasPermission = true
			}

			if !hasPermission {
				tableMutex.Unlock()
				time.Sleep(10 * time.Millisecond)
			} else {
				tableMutex.Unlock()
			}
		}

		// Еда
		eatTime := 100 + rng.Intn(201)
		time.Sleep(time.Millisecond * time.Duration(eatTime))

		if verbose && i < 10 {
			fmt.Printf("Философ %d ест спагетти (итерация %d)\n", id, i+1)
		}

		// Возврат вилок
		tableMutex.Lock()
		forksAvailable[leftFork] = true
		forksAvailable[rightFork] = true
		tableMutex.Unlock()
	}
}

// реализует философа с иерархией ресурсов
func (dp *DiningPhilosophers) philosopherResourceHierarchy(id, iterations int, verbose bool, forks []*sync.Mutex, wg *sync.WaitGroup) {
	defer wg.Done()

	rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(id)))

	// Присваиваем порядковые номера вилкам
	firstFork, secondFork := id, (id+1)%dp.numPhilosophers

	// Для последнего философа меняем порядок
	if id == dp.numPhilosophers-1 {
		firstFork, secondFork = 0, id
	}

	// Убедимся, что firstFork имеет меньший номер
	if firstFork > secondFork {
		firstFork, secondFork = secondFork, firstFork
	}

	for i := 0; i < iterations; i++ {
		// Размышление
		thinkTime := 50 + rng.Intn(151)
		time.Sleep(time.Millisecond * time.Duration(thinkTime))

		if verbose && i < 10 {
			fmt.Printf("Философ %d размышляет (итерация %d)\n", id, i+1)
		}

		// Захват вилок в порядке возрастания номеров
		forks[firstFork].Lock()
		forks[secondFork].Lock()

		// Еда
		eatTime := 100 + rng.Intn(201)
		time.Sleep(time.Millisecond * time.Duration(eatTime))

		if verbose && i < 10 {
			fmt.Printf("Философ %d ест спагетти (итерация %d)\n", id, i+1)
		}

		// Освобождение вилок
		forks[secondFork].Unlock()
		forks[firstFork].Unlock()
	}
}


func (dp *DiningPhilosophers) RunSimulation(iterations int, verbose bool) {
	var wg sync.WaitGroup

	strategyName := ""
	switch dp.strategy {
	case MutexStrategy:
		strategyName = "Мьютексы"
	case SemaphoreStrategy:
		strategyName = "Семафоры"
	case TryLockStrategy:
		strategyName = "Попытка захвата"
	case ArbitratorStrategy:
		strategyName = "Арбитр"
	case ResourceHierarchyStrategy:
		strategyName = "Иерархия ресурсов"
	}

	fmt.Println("\n=== Задача обедающих философов ===")
	fmt.Printf("Философов: %d\n", dp.numPhilosophers)
	fmt.Printf("Стратегия: %s\n", strategyName)
	fmt.Printf("Итераций: %d\n", iterations)

	if verbose && iterations > 10 {
		fmt.Println("(Вывод ограничен первыми 10 итерациями)")
	}

	// Инициализация ресурсов в зависимости от стратегии
	switch dp.strategy {
	case MutexStrategy:
		forks := make([]*sync.Mutex, dp.numPhilosophers)
		for i := 0; i < dp.numPhilosophers; i++ {
			forks[i] = &sync.Mutex{}
		}

		for i := 0; i < dp.numPhilosophers; i++ {
			wg.Add(1)
			go dp.philosopherMutex(i, iterations, verbose, forks, &wg)
		}

	case SemaphoreStrategy:
		forks := make([]*semaphore.Weighted, dp.numPhilosophers)
		for i := 0; i < dp.numPhilosophers; i++ {
			forks[i] = semaphore.NewWeighted(1)
		}

		for i := 0; i < dp.numPhilosophers; i++ {
			wg.Add(1)
			go dp.philosopherSemaphore(i, iterations, verbose, forks, &wg)
		}

	case TryLockStrategy:
		forks := make([]*sync.Mutex, dp.numPhilosophers)
		for i := 0; i < dp.numPhilosophers; i++ {
			forks[i] = &sync.Mutex{}
		}

		for i := 0; i < dp.numPhilosophers; i++ {
			wg.Add(1)
			go dp.philosopherTryLock(i, iterations, verbose, forks, &wg)
		}

	case ArbitratorStrategy:
		tableMutex := &sync.Mutex{}
		forksAvailable := make([]bool, dp.numPhilosophers)
		for i := 0; i < dp.numPhilosophers; i++ {
			forksAvailable[i] = true
		}

		for i := 0; i < dp.numPhilosophers; i++ {
			wg.Add(1)
			go dp.philosopherArbitrator(i, iterations, verbose, tableMutex, forksAvailable, &wg)
		}

	case ResourceHierarchyStrategy:
		forks := make([]*sync.Mutex, dp.numPhilosophers)
		for i := 0; i < dp.numPhilosophers; i++ {
			forks[i] = &sync.Mutex{}
		}

		for i := 0; i < dp.numPhilosophers; i++ {
			wg.Add(1)
			go dp.philosopherResourceHierarchy(i, iterations, verbose, forks, &wg)
		}
	}

	wg.Wait()
	fmt.Println("\nСимуляция завершена успешно!")
}

// бенчмарк
func (dp *DiningPhilosophers) RunBenchmark(maxPhilosophers, iterations int) {
	fmt.Println("\n=== Бенчмарк задачи обедающих философов ===")

	benchmarkResults := make([]BenchmarkResult, 0)

	strategies := []Strategy{MutexStrategy, SemaphoreStrategy, TryLockStrategy,
		ArbitratorStrategy, ResourceHierarchyStrategy}
	strategyNames := []string{"Мьютексы", "Семафоры", "Попытка захвата",
		"Арбитр", "Иерархия ресурсов"}

	philosopherCounts := []int{5, 10, 20}

	for _, count := range philosopherCounts {
		if count > maxPhilosophers {
			continue
		}

		for s, strategy := range strategies {
			testName := fmt.Sprintf("%d_философов_%s", count, strategyNames[s])

			fmt.Printf("Тестируем: %s... ", testName)

			dp := NewDiningPhilosophers(count, strategy)

			b := NewBenchmark(testName, false)
			dp.RunSimulation(iterations, false)

			time := b.StopSilent()
			benchmarkResults = append(benchmarkResults, BenchmarkResult{
				Name:             testName,
				TimeMicroseconds: time,
				TimeMilliseconds: time / 1000.0,
				TimeSeconds:      time / 1000000.0,
			})

			fmt.Printf("%.2f мкс\n", time)
		}
	}

	SaveToCSV(benchmarkResults, "philosophers_benchmark.csv")
	fmt.Println("\nБенчмарк завершен. Результаты сохранены в philosophers_benchmark.csv")
}


func RunPhilosophers() {
	fmt.Println("\n=== Задание 3: Обедающие философы ===")
	fmt.Println("Классическая задача синхронизации\n")

	var choice string
	fmt.Println("Выберите режим:")
	fmt.Println("1. Стандартная симуляция")
	fmt.Println("2. Расширенный бенчмарк")
	fmt.Print("Ваш выбор: ")
	fmt.Scan(&choice)

	switch choice {
	case "1":
		var numPhilosophers, iterations, strategyChoice int

		fmt.Print("\nВведите количество философов (2-20): ")
		fmt.Scan(&numPhilosophers)

		fmt.Print("Введите количество итераций на философа (1-100): ")
		fmt.Scan(&iterations)

		fmt.Println("\nВыберите стратегию синхронизации:")
		fmt.Println("1. Мьютексы ")
		fmt.Println("2. Семафоры")
		fmt.Println("3. Попытка захвата ")
		fmt.Println("4. Арбитр ")
		fmt.Println("5. Иерархия ресурсов")
		fmt.Print("Ваш выбор: ")
		fmt.Scan(&strategyChoice)

		if numPhilosophers < 2 {
			numPhilosophers = 2
		}
		if numPhilosophers > 20 {
			numPhilosophers = 20
		}
		if iterations < 1 {
			iterations = 1
		}
		if iterations > 100 {
			iterations = 100
		}

		var strategy Strategy
		switch strategyChoice {
		case 1:
			strategy = MutexStrategy
		case 2:
			strategy = SemaphoreStrategy
		case 3:
			strategy = TryLockStrategy
		case 4:
			strategy = ArbitratorStrategy
		case 5:
			strategy = ResourceHierarchyStrategy
		default:
			strategy = MutexStrategy
		}

		dp := NewDiningPhilosophers(numPhilosophers, strategy)

		b := NewBenchmark("Симуляция обедающих философов")
		dp.RunSimulation(iterations, true)
		b.Stop()

	case "2":
		var iterations int
		fmt.Print("\nВведите количество итераций на философа (10-100): ")
		fmt.Scan(&iterations)

		if iterations < 10 {
			iterations = 10
		}
		if iterations > 100 {
			iterations = 100
		}

		RunPhilosophersBenchmark()

	default:
		fmt.Println("Неверный выбор! Запускаю стандартную симуляцию...")
		dp := NewDiningPhilosophers(5, MutexStrategy)
		dp.RunSimulation(10, true)
	}
}

// расширенный бенчмарк
func RunPhilosophersBenchmark() {
	fmt.Println("\n=== Расширенный бенчмарк обедающих философов ===")

	var iterations int
	fmt.Print("Введите количество итераций на философа (10-100): ")
	fmt.Scan(&iterations)

	if iterations < 10 {
		iterations = 10
	}
	if iterations > 100 {
		iterations = 100
	}

	fmt.Println("\nТестируем все стратегии...")

	dp := NewDiningPhilosophers(5, MutexStrategy)
	dp.RunBenchmark(20, iterations)
}
