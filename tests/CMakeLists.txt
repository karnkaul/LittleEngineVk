cmake_minimum_required(VERSION 3.13)

# Hash
add_executable(test-hash hash_test.cpp)
target_link_libraries(test-hash PRIVATE dtest::main levk::core levk::compile-options)
add_test(Hash test-hash)

# StaticAny
add_executable(test-any any_test.cpp)
target_link_libraries(test-any PRIVATE dtest::main levk::core levk::compile-options)
add_test(ktl::fixed_any test-any)

# ECS (Registry)
add_executable(test-decf decf_test.cpp)
target_link_libraries(test-decf PRIVATE levk::core dtasks::dtasks decf::decf levk::compile-options)
add_test(decf::registry test-decf)

# fixed_vector
add_executable(test-fv fixed_vector_test.cpp)
target_link_libraries(test-fv PRIVATE dtest::main levk::core levk::compile-options)
add_test(ktl::fixed_vector test-fv)

# monotonic_map
add_executable(test-mm monotonic_map_test.cpp)
target_link_libraries(test-mm PRIVATE dtest::main levk::core levk::compile-options)
add_test(ktl::monotonic_map test-mm)
