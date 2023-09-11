cmake_minimum_required(VERSION 3.26)
project(2_SEM_lab1 C)

set(CMAKE_C_STANDARD 11)

#add_executable(2_SEM_lab1 threads_creation/threads_creation.c)
#add_executable(2_SEM_lab1 joinable_and_detached/join_example.c)
#add_executable(2_SEM_lab1 joinable_and_detached/thread_creation_in_loop_detach_func.c)
#add_executable(2_SEM_lab1 joinable_and_detached/thread_creation_in_loop_detach_attr.c)
add_executable(2_SEM_lab1 thread_routine_args/thread_routine_args.c)