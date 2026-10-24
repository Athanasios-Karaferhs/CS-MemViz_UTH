@echo off
echo Running Python analysis...
python MemProp.py test_2nd.c malloc_prob.h

echo Compiling C code...
gcc test_2nd.c -o test.exe

echo Build complete!