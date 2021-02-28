#-------------------------------------------------
#
# Project created by QtCreator 2012-12-26T20:57:48
#
#-------------------------------------------------

QT       -= core gui

TARGET = lazyusf
TEMPLATE = lib
CONFIG += staticlib

DEFINES += ARCH_MIN_SSE2

SOURCES += \
    audio.c \
    cpu.c \
    dma.c \
    exception.c \
    interpreter_cpu.c \
    interpreter_ops.c \
    main.c \
    memory.c \
    pif.c \
    registers.c \
    tlb.c \
    usf.c \
    cpu_hle.c \
    audiolib.c \
    os.c \
    rsp/rsp.c \
    rsp_hle/alist.c \
    rsp_hle/alist_audio.c \
    rsp_hle/alist_naudio.c \
    rsp_hle/alist_nead.c \
    rsp_hle/audio_hle.c \
    rsp_hle/cicx105.c \
    rsp_hle/jpeg.c \
    rsp_hle/main_hle.c \
    rsp_hle/memory_hle.c \
    rsp_hle/mp3.c \
    rsp_hle/musyx.c \
    rsp_hle/plugin_hle.c

HEADERS += \
    usf_internal.h \
    tlb.h \
    interpreter_cpu.h \
    main.h \
    opcode.h \
    interpreter_ops.h \
    registers.h \
    types.h \
    cpu_hle.h \
    audiolib.h \
    os.h \
    rsp/execute.h \
    rsp/su.h \
    rsp/rsp.h \
    rsp/vu/vabs.h \
    rsp/vu/vsaw.h \
    rsp/vu/vsubc.h \
    rsp/vu/vcr.h \
    rsp/vu/vrcph.h \
    rsp/vu/vsub.h \
    rsp/vu/vand.h \
    rsp/vu/vmacu.h \
    rsp/vu/vrsql.h \
    rsp/vu/vmudh.h \
    rsp/vu/vmacf.h \
    rsp/vu/divrom.h \
    rsp/vu/vmulu.h \
    rsp/vu/vmrg.h \
    rsp/vu/clamp.h \
    rsp/vu/vch.h \
    rsp/vu/vadd.h \
    rsp/vu/shuffle.h \
    rsp/vu/cf.h \
    rsp/vu/vrcpl.h \
    rsp/vu/vcl.h \
    rsp/vu/vnor.h \
    rsp/vu/vmudl.h \
    rsp/vu/vmacq.h \
    rsp/vu/vmudn.h \
    rsp/vu/vxor.h \
    rsp/vu/vnop.h \
    rsp/vu/vmudm.h \
    rsp/vu/veq.h \
    rsp/vu/vmov.h \
    rsp/vu/vlt.h \
    rsp/vu/vmadm.h \
    rsp/vu/vge.h \
    rsp/vu/vaddc.h \
    rsp/vu/vmadn.h \
    rsp/vu/vmulf.h \
    rsp/vu/vmadl.h \
    rsp/vu/vmadh.h \
    rsp/vu/vu.h \
    rsp/vu/vor.h \
    rsp/vu/vnand.h \
    rsp/vu/vne.h \
    rsp/vu/vnxor.h \
    rsp/vu/vrsq.h \
    rsp/vu/vrsqh.h \
    rsp/vu/vrcp.h \
    rsp/config.h \
    rsp_hle/alist.h \
    rsp_hle/alist_internal.h \
    rsp_hle/arithmetics.h \
    rsp_hle/audio_hle.h \
    rsp_hle/cicx105.h \
    rsp_hle/jpeg.h \
    rsp_hle/main_hle.h \
    rsp_hle/memory_hle.h \
    rsp_hle/musyx.h \
    rsp_hle/plugin_hle.h \
    cpu.h \
    rsp.h \
    memory.h \
    audio.h \
    exception.h \
    pif.h \
    usf.h \
    dma.h \
    config.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

