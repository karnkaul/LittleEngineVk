cmake_minimum_required(VERSION 3.13)

# Hash
add_executable(test-hash hash_test.cpp)
target_link_libraries(test-hash PRIVATE levk-core)
add_test(Hash test-hash)

# ECS (Registry)
add_executable(test-ecs ecs_test.cpp)
target_link_libraries(test-ecs PRIVATE levk-engine)
add_test(ECS test-ecs)
