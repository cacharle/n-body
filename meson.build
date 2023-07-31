project(
  'n-body',
  'c',
  default_options : [
    'c_std=c11',
    'warning_level=everything',
    'optimization=3',
  ],
)
# add_project_arguments('-pg', language : 'c')
# add_project_arguments('-g', language : 'c')
add_project_arguments('-O3', language : 'c')
sdl2_dependency = dependency('sdl2')
sdl2_gfx_dependency = dependency('SDL2_gfx')
sdl2_ttf_dependency = dependency('SDL2_ttf')
cc = meson.get_compiler('c')
math_dependency = cc.find_library('m', required : true)
include_dir = include_directories('src')
subdir('src')
executable(
  'n-body',
  sources,
  include_directories : include_dir,
  dependencies : [sdl2_dependency, sdl2_gfx_dependency, sdl2_ttf_dependency, math_dependency],
)
