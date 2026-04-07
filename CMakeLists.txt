cmake_minimum_required(VERSION 3.10)
project(calcul)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Основная программа
add_executable(calcul
        main.cpp

)

# Тестовый раннер
add_executable(test_runner
        test_runner.cpp

)

# Для удобства - target для тестов
add_custom_target(run_tests
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_runner
        DEPENDS test_runner
        COMMENT "Running tests..."
)
