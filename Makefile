# Makefile для сборки Аудио Плеера
# Использование: mingw32-make [target]
# 
# Доступные цели:
#   build    - пересобрать проект (по умолчанию)
#   clean    - очистить объектные файлы
#   run      - собрать и запустить приложение
#   rebuild  - полная пересборка

BUILD_DIR = build
OBJ_FILE = CMakeFiles/AudioPlayer.dir/src/mainwindow.cpp.obj
EXE_FILE = AudioPlayer.exe

.PHONY: all build clean run rebuild help

# Цель по умолчанию
all: build

# Пересборка проекта
build:
	@if [ -f "$(BUILD_DIR)/$(OBJ_FILE)" ]; then rm -f "$(BUILD_DIR)/$(OBJ_FILE)"; fi
	@cd $(BUILD_DIR) && mingw32-make

# Очистка объектных файлов
clean:
	@rm -f $(BUILD_DIR)/CMakeFiles/AudioPlayer.dir/src/*.obj 2>/dev/null || true

# Полная пересборка
rebuild: clean build

# Собрать и запустить
run: build
	@cd $(BUILD_DIR) && ./$(EXE_FILE)

# Помощь
help:
