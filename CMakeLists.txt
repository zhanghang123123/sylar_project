cmake_minimum_required(VERSION 2.8)
project(sylar)

set(CMAKE_VERBOSE_MAKEFILE ON) 
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")


#messsage( "PROJECT_BINARY_DIR **", ${PROJECT_BINARY_DIR} )

###### 添加yaml-cpp的库相关的 CMakeList信息
include_directories(${PROJECT_SOURCE_DIR}/include)          # 使用include_directories()来指定第三方库的头文件所在的目录。这里是为了添加yaml-cpp的头文件
link_directories(${PROJECT_SOURCE_DIR}/lib)                 # link_directories()来指定库文件的搜索路径。该lib文件夹下有你刚刚复制的3个yaml-cpp的so库

#find_library(YAMLCPP NAMES yaml-cpp)   # YAMLCPP-NOTFOUND
#message("***", ${YAMLCPP})
######

set(LIB_SRC
    sylar/log.cc
    sylar/util.cc
#    sylar/singleton.h
    sylar/config.cc                                  # 因为这里写成了sylar/config.h，导致后面 所有的实现在.cc中的函数等都报error：undefined reference to
    sylar/thread.cc
    )

# 5.制作动/静态库[add_library是制作的意思, 库名称:sylar， SHARED 是库类型:动态库,还可以是 STATIC静态库, 动态库的源文件是*.cpp等]
add_library(sylar SHARED ${LIB_SRC})
# add_library(sylar_static STATIC ${LIB_SRC})
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar")

# test
set(TARGET_Test test)
add_executable(${TARGET_Test} tests/test.cc)
#add_dependencies(${TARGET_Test} sylar)
target_link_libraries(${TARGET_Test} sylar yaml-cpp pthread)

# test_config
add_executable(test_config tests/test_config.cc)
target_link_libraries(test_config sylar yaml-cpp pthread)

# test_thread
set(TARGET_Thread test_thread)
add_executable(${TARGET_Thread} tests/test_thread.cc)
target_include_directories(${TARGET_Thread} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${TARGET_Thread} sylar yaml-cpp pthread)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)  # 5.通过set方式指定库文件的生成路径(LIBRARY_OUTPUT_PATH这个宏就是指定库文件的生成路径)


# PROJECT_SOURCE_DIR (等价于CMakeLists.txt所在的路径)
# PROJECT_SOURCE_DIR的默认值为项目根目录的路径（CMakeLists.txt所在的路径）。如果你使用的是子目录CMakeLists.txt，那么PROJECT_SOURCE_DIR将会是子目录的路径。

# CMAKE_SOURCE_DIR指向最顶层的CMakeLists.txt文件所在的目录，即整个项目的根目录，它非常适合用于定义整个项目范围内的源代码路径
# PROJECT_SOURCE_DIR和CMAKE_SOURCE_DIR都是指向项目源代码目录的变量，但在多项目的情况下，它们的值可能不同。
# PROJECT_SOURCE_DIR更具体地指向当前CMakeLists.txt文件所在的目录，而CMAKE_SOURCE_DIR则指向整个项目的根目录。

# target_link_libraries(my_program xxlib x1lib)
# 用于指定一个目标（例如一个可执行文件或库）在链接时需要链接哪些库。my_program 是我们的可执行文件目标，xxlib 等(可以是多个)是我们需要链接的库。
# 如果xxlib等中的一个是系统库，CMake 或链接器会在标准的库路径（如 /usr/lib, /usr/local/lib 等）中查找它。

#
