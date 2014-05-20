project(dfhack-libdasm)
ADD_LIBRARY(dfhack-libdasm STATIC EXCLUDE_FROM_ALL libdasm.c libdasm.h)
IDE_FOLDER(dfhack-libdasm "Depends")