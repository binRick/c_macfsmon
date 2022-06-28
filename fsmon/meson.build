fsmon_srcs = [
  'fsmon.c',
]
fsmon_inc_dirs = [
  '.',
  inc,
]
fsmon_deps = [
  fsmatch_dep,
]
fsmon_lib = static_library('fsmon',
  fsmon_srcs,
  install: true,
  dependencies: fsmon_deps,
  include_directories: fsmon_inc_dirs,
)

fsmon_dep = declare_dependency(
  include_directories: fsmon_inc_dirs,
  link_with: fsmon_lib,
)
