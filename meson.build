project(
  'n-body',
  'c',
  default_options : ['c_std=c99'],
)
add_global_arguments('-g', language : 'c')
sdl2_dependency = dependency('sdl2')
cc = meson.get_compiler('c')
math_dependency = cc.find_library('m', required : true)
include_dir = include_directories('src')
subdir('src')
executable('n-body', sources, include_directories : include_dir, dependencies : [sdl2_dependency, math_dependency])
