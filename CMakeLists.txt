cmake_minimum_required(VERSION 2.8)
project(sylar)

set(CMAKE_VERBOSE_MAKEFILE ON) 
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

###### 添加yaml-cpp的库相关的 CMakeList信息
include_directories(.)                                                  # 这个不知道什么意思
include_directories(/home/hangzhang/projects/sylar_project/include)     # 这个路径需要按照你自己主机上include放的位置
link_directories(/home/hangzhang/projects/sylar_project/lib)            # 同样，根据自己主机lib的位置，必须保证该lib文件夹下有你刚刚复制的3个yaml-cpp的so库

find_library(YAMLCPP yaml-cpp)                                     
# message("***", ${YAMLCPP})
######

set(LIB_SRC
    sylar/log.cc
    sylar/util.cc
#    sylar/singleton.h
    sylar/config.cc                                  # 因为这里写成了sylar/config.h，导致后面 所有的实现在.cc中的函数等都报error：undefined reference to
    )

add_library(sylar SHARED ${LIB_SRC})
# add_library(sylar_static STATIC ${LIB_SRC})
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar")

# test
add_executable(test tests/test.cc)
add_dependencies(test sylar)
target_link_libraries(test sylar -L/home/hangzhang/projects/sylar_project/lib -lyaml-cpp)
# target_link_libraries(test sylar ${YAMLCPP})

# test_config
add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
# target_link_libraries(test_config sylar)    添加yaml-cpp 之前
target_link_libraries(test_config sylar -L/home/hangzhang/projects/sylar_project/lib -lyaml-cpp)      
# target_link_libraries(test_config sylar ${YAMLCPP})           

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

