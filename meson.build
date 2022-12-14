project('fun', ['c'], license: 'MIT', version: '0.1', default_options: [ 'buildtype=release', 'strip=true' ])

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
fun_dep_dep = [
  dependency('spng'),
]
fun_lib = static_library('fun', fun_src, override_options: fun_opt, dependencies: fun_dep_dep + [ cc.find_library('m', required: false) ])
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
# Games
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
if 'games' in get_option('demos') or 'all' in get_option('demos')
  games_dep = [
    fun_dep,
    dependency('glew'),
    dependency('sdl2', default_options: [ 'test=false' ]),
  ]
  executable('games-test', 'games/test.c', dependencies: games_dep, override_options: fun_opt)
endif

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# OpenGL
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
if 'opengl' in get_option('demos') or 'all' in get_option('demos')
  opengl_dep = [
    fun_dep,
    dependency('cgltf'),
    dependency('glew'),
    dependency('sdl2', default_options: [ 'test=false' ]),
  ]
  executable('opengl-cube', 'opengl/cube.c', dependencies: opengl_dep, override_options: fun_opt)
  executable('opengl-gltf', 'opengl/gltf.c', dependencies: opengl_dep, override_options: fun_opt)
endif

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# Vulkan
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
if 'vulkan' in get_option('demos') or 'all' in get_option('demos')
  vulkan_dep = [
    fun_dep,
    dependency('sdl2', default_options: [ 'use_video_vulkan=enabled', 'test=false' ]),
    dependency('Vulkan'),
  ]
  executable('vulkan-cube', 'vulkan/cube.c', dependencies: vulkan_dep, override_options: fun_opt)
endif
