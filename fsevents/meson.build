fsevents_srcs = [
  'fsevents.c',
]
fsevents_inc_dirs = [
  '.',
  inc,
]
fsevents_deps = [
  fsmon_dep,
]
fsevents_lib = static_library('fsevents',
  fsevents_srcs,
  install: true,
  dependencies: fsevents_deps,
  include_directories: fsevents_inc_dirs,
)

fsevents_dep = declare_dependency(
  include_directories: fsevents_inc_dirs,
  link_with: fsevents_lib,
)
