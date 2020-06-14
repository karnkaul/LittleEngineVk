cmake_minimum_required(VERSION 3.13)

# ECS (Registry)
add_executable(test-ecs ecs_test.cpp)
target_link_libraries(test-ecs PRIVATE levk-engine)
add_test(ECS test-ecs)
