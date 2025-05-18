
# types proto file
get_filename_component(types_proto_file "src/proto/ticker_info.proto" ABSOLUTE)
get_filename_component(types_proto_path "${types_proto_file}" PATH)

# Generated sources
set(types_proto_source_files "${CMAKE_SOURCE_DIR}/src/protocol/ticker_info.pb.cc")
set(types_proto_header_files "${CMAKE_SOURCE_DIR}/src/protocol/ticker_info.pb.h")

add_custom_command(
    OUTPUT "${types_proto_source_files}" "${types_proto_header_files}"
    COMMAND ${_PROTOBUF_PROTOC}
    # ARGS --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
    ARGS --cpp_out "${CMAKE_SOURCE_DIR}/src/protocol"
        --proto_path "${types_proto_path}"
        "${types_proto_file}"
    DEPENDS "${types_proto_file}"
    VERBATIM
)

# types_proto_files
add_library(project_types_proto STATIC
    ${types_proto_source_files}
    ${types_proto_header_files}
)
