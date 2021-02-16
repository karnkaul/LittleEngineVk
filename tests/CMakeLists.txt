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
add_executable(test-decf decf_test.cpp)
target_link_libraries(test-decf PRIVATE levk::core dtasks::dtasks decf::decf levk::interface)
add_test(decf::registry_t test-decf)

# fixed_vector
add_executable(test-fv fixed_vector_test.cpp)
target_link_libraries(test-fv PRIVATE levk::core levk::interface)
add_test(kt::fixed_vector_t test-fv)
