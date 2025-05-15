default_target: build

BUILD_ROOT := build
MODE := Debug

clean:
	rm -rf build

init:
	@if ! [ -d build ]; then\
			cmake --no-warn-unused-cli -D CMAKE_CXX_COMPILER=/usr/bin/clang++ -S . -B $(BUILD_ROOT) -G 'Ninja Multi-Config';\
	fi
.PHONY: init

build: init src/**/*.cpp
	cmake --build $(BUILD_ROOT) --config $(MODE)
.PHONY: build

run-server: build
	./build/Debug/redis
.PHONY: debug

run-client: build
	./build/Debug/redis_client
.PHONY: client

test: build
	cd build && ctest --output-on-failure
.PHONY: test
