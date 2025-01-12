cmake_minimum_required(VERSION 3.10)

# Имя проекта
project(GenericNetlinkApp)

# Стандарт C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Подключаем libnl
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNL REQUIRED libnl-3.0 libnl-genl-3.0)

# Путь к заголовкам
include_directories(
        ${LIBNL_INCLUDE_DIRS}
        ${NLOHMANN_JSON_INCLUDE_DIR}
)

# Линковка библиотек
link_directories(${LIBNL_LIBRARY_DIRS})

# Клиент и сервер
add_executable(server server/app.cpp server/server.cpp)
add_executable(client client/app.cpp client/client.cpp)

# Линкуем libnl к клиенту и серверу
target_link_libraries(server ${LIBNL_LIBRARIES})
target_link_libraries(client ${LIBNL_LIBRARIES})

# Запуск скрипта auto_format.sh
add_custom_target(run_auto_format
        COMMAND ${CMAKE_COMMAND} -E echo "Running auto_format.sh"
        COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_SOURCE_DIR}/auto_format.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)


# Цель prebuild
add_custom_target(prebuild ALL
        DEPENDS run_auto_format
)

# Зависимость сборки `server` и `client` от `prebuild`
add_dependencies(server prebuild)
add_dependencies(client prebuild)