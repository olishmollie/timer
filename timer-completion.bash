_timer() {
  local root cur prev options commands timers
  COMPREPLY=()
  root=$HOME/.timer
  cur="${COMP_WORDS[COMP_CWORD]}"
  prev="${COMP_WORDS[COMP_CWORD - 1]}"
  commands="start stop status list"
  timers="$(basename $root/*)"

  case "${prev}" in
    -n)
      COMPREPLY=( $(compgen -W "${timers}" -- ${cur}) )
      return 0;;
    -d)
      COMPREPLY=( $(compgen -W "${timers}" -- ${cur}) )
      return 0;;
    *)
      COMPREPLY=( $(compgen -W "${commands}" -- ${cur}) )
      return 0;;
    esac

}
complete -F _timer timer

