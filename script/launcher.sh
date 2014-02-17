#!/bin/bash

main(){
    # pre {{{
    local mode

    mode="$1"
    keystrokes_history="$HOME/.workrave/todaystats"

    if [[ ! -e "${keystrokes_history}" ]] ; then
	echo -e "run workrave first"
	exit 1
    fi

    # Usage
    if [[ -z "${1}" ]] ; then
	echo -e "Usage: $(basename $BASH_SOURCE) mode"
	echo -e "modes:"
	echo -e "keystrokes"
	exit 1
    fi

    # }}}

    case $mode in
	keystrokes)

	    while inotifywait -q -e modify "${keystrokes_history}"
	    do
		tail -1 "${keystrokes_history}" | awk '{print $8}'
		# that file is not inmediately writed but in any case we add a little pause for not enter in possible infinite loops
		sleep 1
	    done

	    ;;

	*)
	    echo -e "option not implemented"
	    exit 1
	    ;;
    esac

}

#
#  MAIN
#
main "$@"

# vim: set foldmethod=marker :

