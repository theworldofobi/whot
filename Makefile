.PHONY: all build clean test run install help

BUILD_DIR = build
BUILD_TYPE ?= Debug

all: build

help:
	@echo "Whot Game - Build Commands"
	@echo "==========================="
	@echo "make build       - Build the project"
	@echo "make clean       - Clean build files"
	@echo "make test        - Run tests"
	@echo "make run         - Run the server"
	@echo "make install     - Install the application"
	@echo "make debug       - Build in debug mode"
	@echo "make release     - Build in release mode"
	@echo "make docs        - Generate documentation"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

configure: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ..

build: configure
	cd $(BUILD_DIR) && cmake --build . -j$(nproc)

debug:
	$(MAKE) build BUILD_TYPE=Debug

release:
	$(MAKE) build BUILD_TYPE=Release

clean:
	rm -rf $(BUILD_DIR)

test: build
	cd $(BUILD_DIR) && ctest --output-on-failure

run: build
	./$(BUILD_DIR)/bin/whot_server

install: build
	cd $(BUILD_DIR) && cmake --install .

docs:
	cd $(BUILD_DIR) && cmake .. -DBUILD_DOCS=ON && make docs

format:
	find src include -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i

lint:
	find src include -name '*.cpp' -o -name '*.hpp' | xargs cppcheck --enable=all

rebuild: clean build

package: release
	cd $(BUILD_DIR) && cpack

.DEFAULT_GOAL := help
