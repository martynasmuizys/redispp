
set(Tests
    serialize_test.cpp
    deserialize_test.cpp
)

include_directories(../src)
add_executable(serialize_test serialize_test.cpp ../src/core/parser.cpp)
add_executable(deserialize_test deserialize_test.cpp ../src/core/parser.cpp)

add_test(Test_0 ./Debug/serialize_test)
add_test(Test_1 ./Debug/deserialize_test)
