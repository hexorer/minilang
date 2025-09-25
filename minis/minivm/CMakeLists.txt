add_executable(minivm main.cpp)
target_sources(minivm PRIVATE vm.h vm.cpp)
target_link_libraries(minivm PRIVATE ${PROJECT_NAME})
