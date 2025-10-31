cmake_minimum_required(VERSION 4.0)
project(rome C)

set(CMAKE_C_STANDARD 11)

add_executable(rome main.c
        result.h
        result.c
        rome.h
        rome.c)
