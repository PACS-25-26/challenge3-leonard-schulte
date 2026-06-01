[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/tKSbaXxd)

# Challenge3
The third challenge

## Build from source
To build from source with cmake:\
`cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`\
`cmake --build build -j`

## Run Examples
To run the solver execute:\
`<path to main> <input-parameter-file>`

e.g. for the default case from the root directory:\
`./bin/main examples/verification_case.ini`

The results are saved as `<case name>.vtk` in the current directory.

## VTK Output
The results are exported to VTK Simple Legacy Fomat.
Dataset Format is "Structured Points" and Dataset Attribute "Scalars".

## Documentation

run `doxygen docs/Doxyfile` in root directory to create doxygen documentation.