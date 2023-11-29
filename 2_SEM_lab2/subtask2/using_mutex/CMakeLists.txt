cmake_minimum_required(VERSION 3.10)

project(2_SEM_lab2 C)

add_executable(main main.c list.c)

target_include_directories(main PUBLIC ..)

set_property(TARGET main PROPERTY C_STANDARD 11)
