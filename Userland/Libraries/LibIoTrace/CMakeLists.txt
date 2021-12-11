compile_ipc(IoTraceFormat.ipc IoTraceFormat.h)

set(SOURCES
    Main.cpp
)
set(GENERATED_SOURCES
    IoTraceFormat.h
)

serenity_lib(LibIoTrace iotrace)
target_link_libraries(LibIoTrace LibC) # FIXME?
#target_link_libraries(LibIoTrace LibIPC)
