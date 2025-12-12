BUILD_DIR = build
OBJ_FILE = CMakeFiles/AudioPlayer.dir/src/mainwindow.cpp.obj
EXE_FILE = AudioPlayer.exe

.PHONY: all build clean run rebuild

all: build

build:
	@if [ -f "$(BUILD_DIR)/$(OBJ_FILE)" ]; then rm -f "$(BUILD_DIR)/$(OBJ_FILE)"; fi
	@cd $(BUILD_DIR) && mingw32-make

clean:
	@rm -f $(BUILD_DIR)/CMakeFiles/AudioPlayer.dir/src/*.obj 2>/dev/null || true

rebuild: clean build

run: build
	@cd $(BUILD_DIR) && ./$(EXE_FILE)
