.PHONY: all build run clean test benchmark deps fmt vet lint dev full

all: build

build:
	go build -o program .

run: build
	./program

clean:
	rm -f program
	rm -f *.csv
	rm -f *.log

test:
	go test -v ./...

benchmark:
	go test -bench=. ./...

deps:
	go mod tidy
	go get golang.org/x/sync

fmt:
	go fmt .

vet:
	go vet .

lint: fmt vet

dev: lint build run

full: clean deps lint test build run
