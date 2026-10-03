cli_args = static_library('flashux-cli-args', 'cli_args.c', dependencies: flashux_dep)
cli_args_dep = declare_dependency(link_with: cli_args, dependencies: flashux_dep)
cli_batch = static_library('flashux-cli-batch', 'cli_batch.c', dependencies: cli_args_dep)
cli_interactive = static_library('flashux-cli-interactive', 'cli_interactive.c',
  dependencies: [cli_args_dep, curses])
cli = executable('flashux-cli', 'main.c', link_with: [cli_batch, cli_interactive],
  dependencies: [cli_args_dep, curses], install: true)
