# CMakeLists.txt robusto para User Configs
cmake_minimum_required(VERSION 3.13.1)

# Buscar todos los archivos .c en este directorio
FILE(GLOB MY_SOURCES "*.c")

# Añadirlos al target 'app' de Zephyr
target_sources(app PRIVATE ${MY_SOURCES})