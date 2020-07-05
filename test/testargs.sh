#!/bin bash

# Argument handling support
# Usage:
# - declare parameters with default values:
#      declare -A ARGS=( ["<arg-name>"]=<default-value> , ... )
# - source this file
# - refer to argument values directly via
#      ${ARGS["<arg-name>"]}

if [ "${BASH_SOURCE[0]}" -ef "$0" ]
then
    echo "This script should not be directly executed!"
    exit 1
fi

function print_usage {
    echo "$0 usage:"
    echo "All arguments are optional, below defaults are in effect for omitted ones."
    echo "Syntax: --<arg-name>=<arg-value>; value defaults to \"yes\" if omitted."
    for arg in ${!ARGS[@]} ; do
        printf "   --%-16s %-32s\n" "$arg" "${ARGS[$arg]}"
    done | sort
}

while [[ $# -gt 0 ]] ; do
    key="$1"; shift
    found=0
    for arg in ${!ARGS[@]} ; do
        case $key in
            --$arg)
                ARGS["$arg"]="yes"
                found=1
                break
                ;;
            --$arg=*)
                pos=$(($(echo $arg | wc -c) + 3))
                ARGS["$arg"]="$(echo $key | cut -c $pos-)"
                found=1
                break
                ;;
        esac
    done
    if [ $found -eq 0 ] ; then
        echo "Unrecognized argument: $key"
        print_usage
        exit 1
    fi
done
