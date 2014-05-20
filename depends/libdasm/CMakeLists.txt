project(libdasm)
ADD_LIBRARY(libdasm STATIC EXCLUDE_FROM_ALL libdasm.c libdasm.h)
IDE_FOLDER(libdasm "Depends")