rem dumpbin /exports c:\windows\system32\opencl.dll
lib /def:opencl.def /out:opencl.lib /MACHINE:x64
git clone https://github.com/KhronosGroup/OpenCL-Headers.git