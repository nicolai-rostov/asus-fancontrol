#!/bin/bash
#
#  asus-fancontrol -- Asus UX31A fan speed regulation
#  Copyright (C) 2013  Nicolai Rostov
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#
# Based on code from:
# http://www.aneas.org/knowledge/asus_f3jp_fan_control.php

set -u

declare -i INTERVAL=5
declare -i MINTEMP=50
declare -i MAXTEMP=80
declare    FANCTRL=/usr/local/bin/fanctrl
declare    LOG_FILE=/var/log/asus-fancontrol.log

ME=$(basename "$0")
DESCRIPTION="Asus UX31A fan speed regulation"
SYNOPSIS="[options]"
OPTIONS_SPEC="\
 Options:

  --interval SEC   Seconds before reading the temp (Default: $INTERVAL)

  --mintemp TEMP   Set fan to minimum speed below this temp (Default: $MINTEMP)
  --maxtemp TEMP   Set fan to maximum speed over this temp (Default: $MAXTEMP)

  --fanctrl PATH   Path to fanctrl (Default: $FANCTRL)

  --log            Log events
  --no-log         Do not log events (Default)

  --log-file PATH  Log file (Default: $LOG_FILE)

  -h --help        This help message
  -v --verbose     Be more verbose"

declare -i MINSPEED=1
declare    MAXSPEED=auto
declare -a TEMPS=(  75  70 65 60)
declare -a SPEEDS=(128 104 88 72)

declare -i VERBOSE=
declare -i LOG=
declare    PID_FILE=/run/asus-fancontrol.pid
declare    SYSTEM_CONFIG_FILE=/etc/asus-fancontrol.conf

# --------------------------------- options -----------------------------------

declare OPTIONS_INTERVAL
declare OPTIONS_MINTEMP
declare OPTIONS_MAXTEMP
declare OPTIONS_FANCTRL
declare OPTIONS_VERBOSE
declare OPTIONS_LOG
declare OPTIONS_LOG_FILE

while test $# -ne 0
do
        case "$1" in
        --show-options)       echo    "--help --verbose --interval --mintemp --maxtemp --fanctrl --log-file --log --no-log"; exit;;
        --synopsis)           echo    "$ME $SYNOPSIS"; exit;;
        --help|-h)            echo -e "$DESCRIPTION\nUsage: $ME $SYNOPSIS\n\n$OPTIONS_SPEC\n"; exit;;
        --verbose|-v)         OPTIONS_VERBOSE+=1;;
        --quiet|--no-verbose) OPTIONS_VERBOSE=0;;
        --interval)           OPTIONS_INTERVAL=$2; shift;;
        --mintemp)            OPTIONS_MINTEMP=$2;  shift;;
        --maxtemp)            OPTIONS_MAXTEMP=$2;  shift;;
        --fanctrl)            OPTIONS_FANCTRL=$2;  shift;;
        --log-file)           OPTIONS_LOG_FILE=$2; shift;;
        --log)                OPTIONS_LOG+=1;;
        --no-log)             OPTIONS_LOG=0;;
        --)                   shift; break;;
        -*)                   echo "Unknown option: $1" >&2; exit 1;;
        *)                    break;;
        esac
        shift
done

# -------------------------------- functions ----------------------------------

verbose () {
        [ $VERBOSE -gt 0 -a $# -gt 0 ] &&\
                echo -e "$ME: $1" >&2
        return 0
}

log_it () {
        [ $LOG -gt 0 -a $# -gt 0 ] &&\
               echo "$(date --rfc-3339=seconds): $*" >>"$LOG_FILE" 
        return 0
}

refresh_config () {
        read_config_file
        [ -v OPTIONS_INTERVAL ] && INTERVAL=$OPTIONS_INTERVAL
        [ -v OPTIONS_VERBOSE ]  && VERBOSE=$OPTIONS_VERBOSE
        [ -v OPTIONS_MINTEMP ]  && MINTEMP=$OPTIONS_MINTEMP
        [ -v OPTIONS_MAXTEMP ]  && MAXTEMP=$OPTIONS_MAXTEMP
        [ -v OPTIONS_FANCTRL ]  && FANCTRL=$OPTIONS_FANCTRL
        [ -v OPTIONS_LOG  ]     && LOG=$OPTIONS_LOG
        [ -v OPTIONS_LOG_FILE ] && LOG_FILE=$OPTIONS_LOG_FILE
        return 0
}

read_config_file () {
        [ -r "$SYSTEM_CONFIG_FILE" ] &&\
                . "$SYSTEM_CONFIG_FILE"
        return 0
}

exit_if_not_alone () {
        if [ -r "$PID_FILE" ]
        then
                pid=`cat "$PID_FILE"`
                if [ $$ -ne $pid ]
                then
                        verbose "Found another instance: Exit gracefully"
                        exit 0
                fi
        fi
        return 0
}

get_temperature () {
        local temp
        temp=`cat /sys/class/thermal/thermal_zone0/temp`
        temp=${temp:0:2}
        echo $temp
        return 0
}

set_max_fan_speed () { "$FANCTRL" "$1" >/dev/null; }


# -------------------------------- variables ----------------------------------

if [ "$(id -ru)" -ne 0 ]
then
        echo "abort: You must be root to run this script." >&2
        exit 2
fi

refresh_config

if [ ! -x "$FANCTRL" ]
then
        echo "abort: Couldn't find $(basename "$FANCTRL")" >&2
        exit 2
fi

verbose "interval: $INTERVAL"
verbose "mintemp: $MINTEMP"
verbose "maxtemp: $MAXTEMP"
verbose "minspeed: $MINSPEED"
verbose "maxspeed: $MAXSPEED"
verbose "temps: ${TEMPS[*]}"
verbose "speeds: ${SPEEDS[*]}"
verbose "fanctrl: $FANCTRL"
verbose "log: $LOG"
verbose "log_file: $LOG_FILE"

# ---------------------------------- main -------------------------------------

echo $$ >"$PID_FILE"

declare -i old_temp=0 temp=0
declare    old_speed= speed=$MINSPEED
declare -i pid

while true
do
        exit_if_not_alone
        refresh_config

        temp=`get_temperature`

        if [ $temp -ne $old_temp ]
        then
                old_temp=$temp
                verbose "temp: $temp C"
        fi

        if   [ $temp -le $MINTEMP ]; then speed=$MINSPEED
        elif [ $temp -ge $MAXTEMP ]; then speed=$MAXSPEED
        else
                for ((i=0; i < ${#TEMPS[@]}; i++))
                do
                        [ $temp -lt ${TEMPS[$i]} ] &&\
                                continue
                        if [ -n "${SPEEDS[$i]}" ]
                        then
                                 speed=${SPEEDS[$i]}
                        else
                                 speed=auto
                                 warn "Couldn't find speed[$i]."
                        fi
                        break
                done
        fi

        if [ "$speed" != "$old_speed" ]
        then
                old_speed=$speed
                set_max_fan_speed "$speed"
                if [ $? -eq 0 ]
                then
                        log_it "temp=$temp speed=$speed"
                        verbose "speed: $speed"
                else
                        log_it "temp=$temp: Couldn't set fan speed to $speed"
                        echo "warning: Couldn't set speed to $speed" >&2
                fi
        fi

        sleep $INTERVAL
done
