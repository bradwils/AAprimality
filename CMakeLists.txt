cmake_minimum_required(VERSION 3.16)
project(AAprimality CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(common INTERFACE)
target_include_directories(common INTERFACE include common)
target_compile_options(common INTERFACE -Wall -Wextra)

add_library(fermat       implementations/fermat_primality/fermat.cpp) # CJ
add_library(miller_rabin implementations/miller_rabin/miller_rabin.cpp) # Brad
add_library(aks          implementations/AKS/aks.cpp) # Ben
foreach(lib fermat miller_rabin aks)
  target_link_libraries(${lib} PUBLIC common)
endforeach()
target_include_directories(aks PUBLIC implementations/AKS)

add_executable(primality src/main.cpp) # run algorithms on given numbers
add_executable(tests tests/tests.cpp) # correctness against known results
add_executable(bench bench/bench.cpp) # experiments, CSV into results/
foreach(exe primality tests bench)
  target_link_libraries(${exe} PRIVATE fermat miller_rabin aks)
endforeach()
