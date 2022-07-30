project('c_macfsmon', ['c'],default_options:['warning_level=0','c_std=gnu99'],version: '0.0.1')
cc = meson.get_compiler('c')
pwd = meson.current_source_dir()
meson_deps_dir = join_paths(meson.current_source_dir(),'submodules/meson_deps')
osx_ldflags = get_variable('osx_ldflags', [])
osx_ldflags +=  ['-Wl,-framework,CoreFoundation']
osx_ldflags +=  ['-Wl,-framework,CoreServices']
inc = [include_directories(
   'fsmon',
   'fsmatch',
   'fsevents',
   'submodules/meson_deps/submodules/c_ansi',
)]
osx_args = [
  '-DMACOS=1',
]
#############################################################################
add_project_link_arguments(osx_ldflags, language : ['c'])
add_project_arguments(osx_args, language : ['c'])
#############################################################################
subdir('submodules')
#############################################################################
subdir('fsmatch')
subdir('fsmon')
subdir('fsevents')
subdir('fsmon-test')
subdir('fsmon-chan')
subdir('fsmon-chan-test')
#############################################################################
