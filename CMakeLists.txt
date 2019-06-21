cmake_minimum_required(VERSION 3.14)
project(Naimi_Trehel C)

find_package(MPI REQUIRED)


add_executable(Naimi_Trehel main.c file.c)
target_include_directories(Naimi_Trehel PRIVATE ${MPI_C_INCLUDE_PATH} main.h)
target_compile_options(Naimi_Trehel PRIVATE ${MPI_C_COMPILE_FLAGS} )
target_link_libraries(Naimi_Trehel ${MPI_C_LIBRARIES} ${MPI_C_LINK_FLAGS} )
