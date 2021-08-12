if(NOT TARGET TensorRT::nvinfer)

    find_path(TensorRT_DIR
        NAMES include/NvInfer.h
        PATHS
        /usr/local/TensorRT-7.2.2.3
        /usr
        )

    set(TensorRT_INCLUDE_DIRS ${TensorRT_DIR}/include)

    foreach (lib
            myelin
            nvcaffe_parser
            nvinfer_plugin
            nvinfer
            nvonnxparser
            nvparsers)

        find_library(TensorRT_${lib}_LIBRARY
            ${lib}
            lib${lib}.so.7.2.1
            PATHS
            ${TensorRT_DIR}/lib
            ${TensorRT_DIR}/lib/x86_64-linux-gnu
            ${TensorRT_DIR}/lib64
            )

        add_library(TensorRT::${lib} IMPORTED INTERFACE)

        set_target_properties(TensorRT::${lib} PROPERTIES
            INTERFACE_LINK_LIBRARIES
            ${TensorRT_${lib}_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES
            ${TensorRT_INCLUDE_DIRS}
            )

        list(APPEND TensorRT_LIBRARIES ${TensorRT_${lib}_LIBRARY})

    endforeach()

    message("TensorRT_DIR: ${TensorRT_DIR}")
    message("TensorRT_INCLUDE_DIRS: ${TensorRT_INCLUDE_DIRS}")
    message("TensorRT_LIBRARIES: ${TensorRT_LIBRARIES}")

endif()
