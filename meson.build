project('c_fsmon', ['c'],default_options:['warning_level=0','c_std=gnu99'],
  subproject_dir: 'subprojects',
  )
cc = meson.get_compiler('c')
pwd = meson.current_source_dir()
inc = [
  include_directories(
   'fsmon',
   'fsmatch',
   'fsevents',
  ),
]
osx_ldflags = get_variable('osx_ldflags', [])
osx_ldflags +=  ['-Wl,-framework,CoreFoundation']
osx_ldflags +=  ['-Wl,-framework,CoreServices']
add_project_link_arguments(osx_ldflags, language : ['c'])

add_project_arguments('-DMACOS=1', language : 'c')

#############################################################################
subdir('meson/deps/greatest')
#############################################################################
subdir('fsmatch')
subdir('fsmon')
subdir('fsevents')
subdir('fsmon-test')
