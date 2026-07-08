project(
  'Cosentinii',
  'cpp',
  version : '0.1',
  meson_version : '>= 1.3.0',
  default_options : ['warning_level=2',
                     'cpp_std=c++23',
                     'cpp_eh=none'],
)

dependencies = [
  dependency('sfml-window'),
  dependency('vulkan'),
  dependency('spdlog')
]

vulkan_args = [
  '-DVULKAN_HPP_NO_EXCEPTIONS',
  '-DVULKAN_HPP_NO_CONSTRUCTORS',
  '-DSPDLOG_NO_EXCEPTIONS'
]

sources = [
  'src/cosentinii.cpp',
  'src/vk/primitives.cpp',
  'src/vk/command_buffer.cpp',
  'src/vk/core.cpp',
]

compiler = meson.get_compiler('cpp')
lld = find_program('lld', required: false)

if lld.found()
  message('lld linker will be used.')
  add_project_link_arguments('-fuse-ld=lld', language: 'cpp')
endif

exe = executable(
  'Cosentinii',
  sources,
  install : true,
  dependencies : dependencies,
  cpp_args : vulkan_args,
  cpp_pch : 'src/pch.hpp'

)

test('basic', exe)
