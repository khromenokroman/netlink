cmake_minimum_required(VERSION 3.10)

# Указываем имя проекта
project(GenericNetlinkApp)

# Указываем стандарт C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Подключаем libnl и nlohmann/json
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNL REQUIRED libnl-3.0 libnl-genl-3.0)

# Указываем путь к заголовкам
include_directories(
        ${LIBNL_INCLUDE_DIRS}
        ${NLOHMANN_JSON_INCLUDE_DIR}
)

# Линковка библиотек
link_directories(${LIBNL_LIBRARY_DIRS})

# Добавляем клиент и сервер
add_executable(server server/app.cpp server/server.cpp)
add_executable(client client.cpp)

# Линкуем libnl к клиенту и серверу
target_link_libraries(server ${LIBNL_LIBRARIES})
target_link_libraries(client ${LIBNL_LIBRARIES})