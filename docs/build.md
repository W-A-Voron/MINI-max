# Сборка и запуск

## Зависимости (Ubuntu / Debian)
```bash
sudo apt install build-essential cmake ninja-build git python3 qt6-base-dev libqt6svg6-dev
```
tomlc99 и toml++ подтягиваются автоматически через CMake FetchContent (нужен интернет при первой настройке).

## Сборка (Linux)
```bash
scripts/build.sh            # или: cmake -S . -B build -G Ninja && cmake --build build
scripts/test.sh             # unit + интеграционные тесты
```
Опции CMake: `-DMINIMAX_BUILD_CLIENT=OFF`, `-DMINIMAX_BUILD_SERVER=OFF`, `-DMINIMAX_SANITIZE=ON`.

## Windows (сервер и клиент)
Проверенный путь — [MSYS2](https://www.msys2.org/), оболочка **UCRT64**:
```bash
pacman -S --needed git mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python \
  mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-svg
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
build/server/minimax-server.exe -c configs/server.toml
build/client/minimax-client.exe -c configs/client.toml
```
Готовый zip для Windows (exe + Qt DLL + конфиги + `start-server.bat` / `start-client.bat`) собирает GitHub Actions — артефакт `minimax-windows-x64`.

Сервер на Windows использует Winsock + `WSAPoll`, на Linux — epoll; на macOS/BSD автоматически выбирается poll().
Опция `-DMINIMAX_POLL_BACKEND=ON` включает poll() и на Linux (так тестируется тот же код, что работает на Windows).

## Запуск
```bash
scripts/run-server.sh       # читает configs/server.toml
scripts/run-client.sh       # читает configs/client.toml
```
Сервер всегда запускается с `-c <файл>`; все ключи конфига обязательны.
Клиент без `--config` ищет `client.toml` рядом с бинарником и в `configs/`.
Отладка без дисплея: `QT_QPA_PLATFORM=offscreen minimax-client --screenshot out.png`.

## База данных
Схема: `server/migrations/001_init.sql`, применение: `scripts/migrate.sh <postgres-url>`.
Сервер пока PostgreSQL не использует (подсистема auth/chats — следующий этап).
