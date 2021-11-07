cmake_minimum_required(VERSION 3.13)

add_library(levk-test INTERFACE)
target_link_libraries(levk-test INTERFACE levk::compile-options levk::interface::exe)

# Hash
add_executable(test-hash hash_test.cpp)
target_link_libraries(test-hash PRIVATE dtest::main levk::core levk-test)
add_test(Hash test-hash)

# StaticAny
add_executable(test-any any_test.cpp)
target_link_libraries(test-any PRIVATE dtest::main levk::core levk-test)
add_test(ktl::fixed_any test-any)

# ECS (Registry)
add_executable(test-decf decf_test.cpp)
target_link_libraries(test-decf PRIVATE levk::core dtasks::dtasks decf::decf levk-test)
add_test(decf::registry test-decf)

# ECS (Registry)
add_executable(test-dens dens_test.cpp)
target_link_libraries(test-dens PRIVATE dtest::main dens::dens levk-test)
add_test(dens test-dens)

# fixed_vector
add_executable(test-fv fixed_vector_test.cpp)
target_link_libraries(test-fv PRIVATE dtest::main levk::core levk-test)
add_test(ktl::fixed_vector test-fv)

# delegate
add_executable(test-delegate delegate_test.cpp)
target_link_libraries(test-delegate PRIVATE dtest::main levk::core levk-test)
add_test(ktl::delegate test-delegate)
