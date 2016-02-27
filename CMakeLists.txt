
project(final)			# Название проекта

add_definitions(-std=c++11) 
set(SOURCE_EXE final.cpp)		# Установка переменной со списком исходников для исполняемого файла

set(SOURCE_LIB helper.cpp)			# Тоже самое, но для библиотеки

add_library(foo STATIC ${SOURCE_LIB})	# Создание статической библиотеки с именем foo

add_executable(final ${SOURCE_EXE})	# Создает исполняемый файл с именем main

target_link_libraries(final foo)		# Линковка программы с библиотекой
