include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

lib_LIBRARIES                                                                  = libqcbor.a

nodist_libqcbor_a_SOURCES                                                    = \
    repo/src/ieee754.c                     \
    repo/src/ieee754.h                     \
    repo/src/qcbor_decode.c                   \
    repo/src/qcbor_encode.c                       \
    repo/src/qcbor_err_to_str.c                     \
    repo/src/UsefulBuf.c                   \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am