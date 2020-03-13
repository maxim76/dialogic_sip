cmake_minimum_required (VERSION 3.0)

# Create solution with this name (not project but solution)
project (callserver)

# Build result will be written to
set(EXECUTABLE_OUTPUT_PATH "./bin/windows")

include_directories("C:/Program Files (x86)/Dialogic/HMP/inc")
link_directories("C:/Program Files (x86)/Dialogic/HMP/lib")

# Define projects/executables
add_executable(callserver callserver.cpp)
