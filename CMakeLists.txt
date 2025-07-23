cmake_minimum_required(VERSION 3.31)
project(untitled C)

set(CMAKE_C_STANDARD 11)

add_executable(untitled main.c
        macros.h
        debug.h
        dataStructure.h
        pte.h
        pte.c
        pfn.h
        dataStructure.c
        list.c
        list.h
        pageTrim.c
        pageTrim.h
        writer.c
        writer.h
        util.c
        util.h
        handler.c
        handler.h
        reader.c
        reader.h
)
