cmake_minimum_required(VERSION 3.13)

# Hash
add_executable(test-hash hash_test.cpp)
target_link_libraries(test-hash PRIVATE levk::core levk::interface)
add_test(Hash test-hash)

# StaticAny
add_executable(test-any any_test.cpp)
target_link_libraries(test-any PRIVATE levk::core levk::interface)
add_test(StaticAny test-any)

# ECS (Registry)
add_executable(test-ec ec_test.cpp)
target_link_libraries(test-ec PRIVATE levk::core dtasks::dtasks levk::interface)
add_test(ECS test-ec)

# fixed_vector
add_executable(test-fv fixed_vector_test.cpp)
target_link_libraries(test-fv PRIVATE levk::core levk::interface)
add_test(fixed_vector test-fv)
