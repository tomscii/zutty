#!/usr/bin/env bash

if [ "${BASH_SOURCE[0]}" -ef "$0" ]
then
    echo "This script should not be directly executed!"
    exit 1
fi

# A declaration of parameters and their default values
declare -A ARGS=(
    ["profile"]=zutty
    ["step"]=no          # N.B.: --step=new will only stop at new snaps!
    ["update-sig"]=no
    ["ci-mode"]=no
    ["debug"]=no
)

# Include argument handler
source "testargs.sh"

PROFILE=${ARGS["profile"]}
STEP_MODE=${ARGS["step"]}
UPDATE_SIGNATURE=${ARGS["update-sig"]}
if [ ${PROFILE} == "zutty" ] ; then
    VERIFY_SNAPS=yes
else
    VERIFY_SNAPS=optional
    IMAGE_DIFF_TO=zutty
fi
if [ ${ARGS["ci-mode"]} == "yes" ] ; then
    STEP_MODE=no
    UPDATE_SIGNATURE=no
    EXIT_ON_FAILURE=yes
else
    EXIT_ON_FAILURE=no
fi
if [ ${ARGS["debug"]} == "yes" ] ; then
    DEBUG=yes
else
    DEBUG=no
fi

PLATFORM=$(uname -s)

EXIT_CODE=0

export TEST_NAME=$(basename -s .sh "$0")

export RED="\\e[1;31m"
export GREEN="\\e[1;32m"
export YELLOW="\\e[1;33m"
export WHITE="\\e[1;37m"
export DFLT="\\e[0;39m"
export ERASE_PROMPT="\\eM\\e[J"

function print_error {
    printf "${RED}ERROR: $@${DFLT}\n"
}

function CHECK_DEPS {
    TOOLS="$@"; shift
    MISSING=0
    for t in ${TOOLS} ; do
        which $t >/dev/null
        if [ $? -gt 0 ] ; then
            print_error "Missing dependency: $t"
            MISSING=1
        fi
    done
    if [ $MISSING -gt 0 ] ; then
        exit 1
    fi
}

function CHECK_FILES {
    FILES="$@"; shift
    MISSING=0
    for f in ${FILES} ; do
        if [ ! -f "$f" ] ; then
            print_error "Missing file: $f"
            MISSING=1
        fi
    done
    if [ $MISSING -gt 0 ] ; then
        exit 1
    fi
}

function CHECK_EXE {
    FILES="$@"; shift
    MISSING=0
    for f in ${FILES} ; do
        if [ ! -x "$f" ] ; then
            print_error "Missing executable: $f"
            MISSING=1
        fi
    done
    if [ $MISSING -gt 0 ] ; then
        exit 1
    fi
}

if [ ! -f "profiles/${PROFILE}.sh" ] ; then
    print_error "Could not load profile: ${PROFILE}"
    echo -n "Available profiles:"
    for p in $(ls profiles/*.sh) ; do
        echo -n " $(basename -s .sh $p)"
    done
    echo
    exit 1
fi
source "profiles/${PROFILE}.sh"
echo "Running test: ${TEST_NAME} with profile: ${PROFILE}"
[ -n "${UUT_VERSION}" ] && echo "UUT version: ${UUT_VERSION}"

export TEST_LOG="$(pwd)/output/${PROFILE}/${TEST_NAME}.log"
export UUT_LOG="$(pwd)/output/${PROFILE}/${TEST_NAME}.uut.log"
export UUT_SNAP="$(pwd)/output/${PROFILE}"

export TEST_COUNT=0
export NRES_COUNT=0
export FAIL_COUNT=0

CHECK_DEPS convert compare identify mogrify xrdb xvkbd wmctrl
mkdir -p $(dirname ${UUT_LOG})
mkdir -p ${UUT_SNAP}

CON_PID=$(ps -p $$ -o ppid=)
CON_PID=$(ps -p ${CON_PID} -o ppid=)
CON_WID=$(wmctrl -lp | grep ${CON_PID} | awk '{print $1}')
if [ -z "${CON_WID}" ] ; then
    echo "Console pid: ${CON_PID}"
    printf "        ${YELLOW}wid not found, prompts won't focus console${DFLT}\n"
else
    echo "Console pid: ${CON_PID} wid: ${CON_WID}"
fi

# Add path to installed dependencies, if any
export PATH="$(pwd)/deps/bin":$PATH

# Remove any permanent configuration that could change how Zutty looks/behaves:
[ ${PROFILE} == "zutty" ] && [ -f ${HOME}/.Xresources ] && xrdb -remove

# execute UUT and store its pid
${UUT_EXE} >${UUT_LOG} 2>&1 &
PID=$!
echo "UUT pid: ${PID}"
echo "    log file: ${UUT_LOG}"
echo "    snap dir: ${UUT_SNAP}"

if [ ! -z ${UUT_PID_FROM_NAME} ] ; then
    sleep 0.5
    PID=$(pidof ${UUT_PID_FROM_NAME})
    echo "UUT pid: ${PID}"
fi

# wait until window opens and grab its X window id
WID=''
while true ; do
    if [[ -z "$(ps -p ${PID} -o pid=)" ]] ; then
        echo "UUT process ${PID} gone, aborting."
        exit 1
    fi
    WID=$(wmctrl -lp | grep ${PID} | awk '{print $1}')
    [[ -z "${WID}" ]] || break
done
echo "UUT X window id: ${WID}"

# focus the window
wmctrl -ia "${WID}"
# set it to float (this is i3 specific):
which i3-msg >/dev/null && i3-msg floating enable >/dev/null;

function COUNT_PASS {
    TEST_COUNT=$((TEST_COUNT + 1))
    echo -n "$(test_counters)"
}

function COUNT_FAIL {
    TEST_COUNT=$((TEST_COUNT + 1))
    FAIL_COUNT=$((FAIL_COUNT + 1))
    echo -n "$(test_counters)"
}

function COUNT_NRES {
    TEST_COUNT=$((TEST_COUNT + 1))
    NRES_COUNT=$((NRES_COUNT + 1))
    echo -n "$(test_counters)"
}

function test_counters {
    echo "(${TEST_COUNT}/${NRES_COUNT}/${FAIL_COUNT}) "
}

function FINISH_TEST {
    echo "Total tests: ${TEST_COUNT}  No result: ${NRES_COUNT}  Failed: ${FAIL_COUNT}"
    # display process statistics
    if [ "${PLATFORM}" == "Linux" ] ; then
        ps -p ${PID} -o pid,vsz,rss,trs,sz,time,maj_flt,min_flt,args \
            | tee -a ${TEST_LOG}
        pmap -x ${PID} >> ${TEST_LOG}
        echo "Process memory map appended to ${TEST_LOG}"
    else
        ps -p ${PID} -o pid,vsz,rss,time,args | tee -a ${TEST_LOG}
    fi

    trap - EXIT
    kill "${PID}"
    [ ${PROFILE} == "zutty" ] && [ -f ${HOME}/.Xresources ] && \
        xrdb ${HOME}/.Xresources
    exit ${EXIT_CODE}
}

trap FINISH_TEST EXIT

trap "exit 1" SIGINT

function check_uut {
    if [ $(wmctrl -lp | grep ${WID} | wc -l) -eq 0 ] ; then
        # If IN was interrupted, Return key might have gotten stuck...
        LANG=C xvkbd -window root -no-jump-pointer \
            -text "\{-Return}\D3\{-Return}\D3" 2>/dev/null
        echo "UUT window gone, exiting"
        echo "---------- LOG TAIL ----------"
        tail ${UUT_LOG}

        trap - EXIT
        [ ${PROFILE} == "zutty" ] && [ -f ${HOME}/.Xresources ] && \
            xrdb ${HOME}/.Xresources
        exit 1
    fi
}

function IN {
    local in="$1"; shift
    str=$(echo "$in" | sed 's|\\r|\\D3\\{+Return}\\D3\\{-Return}|');
    check_uut
    LANG=C xvkbd -window ${WID} -no-jump-pointer -delay 50 \
        -text "${str}\\D3" 2>&1 | \
        grep -v "Mode_switch not available as a modifier" | \
        grep -v "AltGr may not work correctly"
}

function FOCUS_CONSOLE {
    if [ ! -z "${CON_WID}" ] ; then
        # focus the console window
        # for some reason, a simple wmctrl -ia does not work;
        # it selects the window but keyboard focus is not active.
        LANG=C xvkbd -window ${CON_WID} -no-back-pointer \
              -text "\{+Control_L}\D1\{-Control_L}\D1" >/dev/null 2>&1
    fi
}

function do_update_sig {
    local name="$1"; shift
    local newsig="$1"; shift
    sed -i -e "s/SNAP ${name} .*$/SNAP ${name} ${newsig}/" "$(basename $0)"
    echo "${name}: signature updated to ${newsig}"
}

function update_sig {
    case ${UPDATE_SIGNATURE} in
        all)
            do_update_sig "$@"
            ;;
        yes)
            FOCUS_CONSOLE
            read -p "Update signature: [y]es / [N]o / [a]ll ? " ans
            printf "${ERASE_PROMPT}"
            case $ans in
                [aA]* ) export UPDATE_SIGNATURE=all; do_update_sig "$@" ;;
                [yY]* ) do_update_sig "$@" ;;
                * ) ;;
            esac
            ;;
        *)
            ;;
    esac
}

function step_prompt {
    FOCUS_CONSOLE
    read -p "[S]tep / [N]ew only / [C]ontinue / [Q]uit (s/n/c/q) ? " ans
    printf "${ERASE_PROMPT}"
    case $ans in
        [sS]* ) STEP_MODE=yes ;;
        [nN]* ) STEP_MODE=new ;;
        [cC]* ) STEP_MODE=no ;;
        [qQ]* ) exit ;;
        * ) ;;
    esac
}

function SNAP {
    local name="$1"; shift
    local refsig="$1"; shift
    local imgfile=${UUT_SNAP}/${name}.png
    check_uut
    xwd -silent -nobdrs -id ${WID} | convert xwd:- png:- > ${imgfile}
    mogrify -strip -taint -compress Lossless ${imgfile}

    case ${VERIFY_SNAPS} in
        yes)
            sig=$(identify -verbose ${imgfile} | grep signature | \
                      awk '{print $2}' | cut -c -32)
            if [ -z "${refsig}" ] ; then
                COUNT_NRES
                printf "${name}: ${YELLOW}NEW${DFLT} ${sig}\n"
                if [ ${STEP_MODE} == "new" ] ; then
                    step_prompt
                fi
            elif [ "${sig}" == "${refsig}" ] ; then
                COUNT_PASS
                printf "${name}: ${GREEN}OK${DFLT}\n"
            else
                COUNT_FAIL
                printf "${name}: ${RED}FAIL${DFLT} sig ${sig} ref ${refsig}\n"
                update_sig ${name} ${sig}
                EXIT_CODE=1
                if [ ${EXIT_ON_FAILURE} == "yes" ] ; then
                    exit
                fi
            fi
            ;;
        optional)
            sig=$(identify -verbose ${imgfile} | grep signature | \
                      awk '{print $2}' | cut -c -32)
            imgerr=${UUT_SNAP}/${name}.error.png
            if [ "${sig}" == "${refsig}" ] ; then
                rm -f ${imgerr}
                COUNT_PASS
                printf "${name}: ${GREEN}MATCH${DFLT}\n"
            else
                COUNT_NRES
                if [ ! -z ${IMAGE_DIFF_TO} ] ; then
                    imgref="$(pwd)/output/${IMAGE_DIFF_TO}/${name}.png"
                    rm -f ${imgerr}
                    AE=$(compare ${imgref} ${imgfile} -metric AE ${imgerr} 2>&1)
                    if [ $? -eq 2 ] ; then
                        printf "${name}: ${WHITE}DONE${DFLT} (different dimensions)\n"
                    else
                        printf "${name}: ${WHITE}DONE${DFLT} (AE=${AE})\n"
                    fi
                else
                    printf "${name}: ${WHITE}DONE${DFLT}\n"
                fi
            fi
            ;;
        *)
            COUNT_NRES
            printf "${name}: ${WHITE}DONE${DFLT}\n"
            ;;
    esac

    if [ ${STEP_MODE} == "yes" ] ; then
        step_prompt
    fi
}

function WAIT_FOR_DOT_COMPLETE {
    while true ; do
        [[ -f .complete ]] && break
        sleep 0.1
    done
    rm -f .complete
}
