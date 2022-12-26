project('fun', ['c'], license: 'MIT', version: '0.1')

cc = meson.get_compiler('c')

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# Common code
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
fun_src = [
  'common/fun.c'
]
fun_inc = [
  'common/'
]
fun_opt = [
  'warning_level=3',
  'c_std=c99'
]
fun_lib = library('fun', fun_src, override_options: fun_opt, dependencies: [ cc.find_library('m', required: false) ])
fun_dep = [
  declare_dependency(link_with: fun_lib, include_directories: fun_inc),
]

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# FFmpeg
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
if 'ffmpeg' in get_option('demos') or 'all' in get_option('demos')
  ffmpeg_dep = [
    fun_dep,
    dependency('libavcodec'),
    dependency('libavformat'),
    dependency('libavutil'),
    dependency('libswscale'),
  ]
  executable('ffmpeg-generate-video', 'ffmpeg/generate_video.c', dependencies: ffmpeg_dep, override_options: fun_opt)
  executable('ffmpeg-remux', 'ffmpeg/remux.c', dependencies: ffmpeg_dep, override_options: fun_opt)
endif

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# OpenGL
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
if 'opengl' in get_option('demos') or 'all' in get_option('demos')
  opengl_dep = [
    fun_dep,
    dependency('glew'),
    dependency('sdl2'),
  ]
  executable('opengl-cube', 'opengl/cube.c', dependencies: opengl_dep, override_options: fun_opt)
endif

