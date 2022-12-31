project('cgltf', 'c', license: 'MIT', version: '1.13')
cgltf_lib = static_library('cgltf', [ 'cgltf_impl.c' ])
cgltf_dep = declare_dependency(link_with: cgltf_lib, include_directories: [ '.' ])
