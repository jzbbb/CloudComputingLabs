#!/bin/bash

##########################################################################################################
#                                                                                                        #
#     Through this script, you can get the score of your implemented kV storage system                   #
#                                                                                                        #
#     Usage: ./lab3_testing.sh [source_folder] [your_sudo_password] [result_folder] [version]            #
#     Example: ./lab3_testing.sh ./Lab3 XXXXXX ./Lab3 1                                                  # 
#                                                                                                        #
#     Author: Fusheng Lin(2019) Bowei Chen(2020) Zequn Cheng(2021) Haoxiang Pan(2023)                    #
#     																									 #
# 	  Advisor: Guo Chen																					 #
#			                                                                                             #
#     Last Modified Date: 2023/5/22                                                                      #
#                                                                                                        #
##########################################################################################################

# Script body starts here

############################### Start of testing parameter settings ########################################

# The parameters received by the script
LAB3_PATH="$1"
PASSWORD="$2"
LAB3_TEST_RESULT_SAVE_PATH="$3"
VERSION="$4"                       # 1: 2pc, 2: raft

if [[ -z "$1" || -z "$2" || -z "$3" || -z "$4" ]]; then
  	echo -e "Usage: ./lab3_testing.sh [source_folder] [your_sudo_password] [result_folder] [version(1:2pc 2:raft)]"
	echo -e "Example: ./lab3_testing.sh ./Lab3 XXXXXX ./Lab3 1"
  	exit 1
fi

# Coordinator's configuration
COORDINATOR_IP=192.168.66.101
COORDINATOR_PORT=8001

# Leader's configuration
follower_ip=(192.168.66.201\
		     192.168.66.202\
			 192.168.66.203)
follower_port=(8001\
			   8002\
			   8003)
LEADER_IP=${follower_ip[0]}
LEADER_PORT=${follower_port[0]}

# Experimental environment configuration
EXECUTABLE_FILE_NAME=""
NC_TIMEOUT=5
ERROR_RETRY_TIMES=3
START_RETYR_TIMES=$[1 * 5]         # 5 seconds
START_COORDINATOR_ONLY=1
START_COORDINATOR_AND_ALL_PARTICIPANTS=2
START_ONE_DATA_PROCESS_SPCECIFIC=3

# Virtual network card environment configuration
DELAY=100 						   # ms
PACKET_LOSS_RATE=10 			   # percentage

LAB3_ABSOLUTE_PATH=""

# System implementation language code
PYTHON=1                           # PYTHON language
C_OR_CPP=2                         # C/C++ language
JAVA=3                             # JAVA language
UNKNOWN_LANGUAGE=255               # UNKNOWN language
NO_JAR_FILE=101              	   # have no jar file
NO_KVSTORE2PCSYSTEM_PY_FILE=102    # have no kvstore2pcsystem.py

# Function return value global variable definition
set_result=""                      # return value of send_set_command
get_result=""					   # return value of send_get_command
printf -v leader_infomation "(.*):(.*)"

# Result code
SUCCESS=0                    	   # operate successfully
FAIL=255                      	   # operate failed
PASSED=66                          # Test passed value
FAILED=99                          # Test failed value

TEST_RESULT_ARR=("")

############################### End of testing parameter settings ##########################################

############################### Start of testing using function definitions ################################

# Set executable file name
function set_executable_file_name
{
	if [ $VERSION -eq 1 ]
	then
		EXECUTABLE_FILE_NAME="kvstore2pcsystem"
	else
		EXECUTABLE_FILE_NAME="kvstoreraftsystem"
	fi
}

# Obtain the absolute path of the specified path
function get_absolute_path
{
	tmp_path="$1"

	# root_dir="/.*"
	if [[ ${tmp_path:0:1} = "/" ]]
	then
		retval=$tmp_path
		echo $retval
	else
		retval=`readlink -f ${tmp_path}`
		echo $retval
	fi
}

# Configuration files(2pc) generated by test scripts
LAB3_ABSOLUTE_PATH=$(get_absolute_path $LAB3_PATH)
coordinator_config_path=${LAB3_ABSOLUTE_PATH}"/coordinator.conf"
participants_config_path=(${LAB3_ABSOLUTE_PATH}"/participant1.conf" \
	                      ${LAB3_ABSOLUTE_PATH}"/participant2.conf" \
	                      ${LAB3_ABSOLUTE_PATH}"/participant3.conf")

# Configuration files(raft) generated by test scripts
followers_config_path=(${LAB3_ABSOLUTE_PATH}"/follower1.conf" \
	                      ${LAB3_ABSOLUTE_PATH}"/follower2.conf" \
	                      ${LAB3_ABSOLUTE_PATH}"/follower3.conf")

# Generate boot information for the storage system
function generate_config_files
{
	if [ $VERSION -eq 1 ]; then
		echo -e "mode coordinator" > ${coordinator_config_path}
		echo -e	"coordinator_info 192.168.66.101:8001" >> ${coordinator_config_path}
		echo -e "participant_info 192.168.66.201:8002" >> ${coordinator_config_path}
		echo -e	"participant_info 192.168.66.202:8003" >> ${coordinator_config_path}
		echo -e	"participant_info 192.168.66.203:8004" >> ${coordinator_config_path}

		echo -e "mode participant" > ${participants_config_path[0]}
		echo -e	"coordinator_info 192.168.66.101:8001" >> ${participants_config_path[0]}
		echo -e	"participant_info 192.168.66.201:8002" >> ${participants_config_path[0]}

		echo -e "mode participant" > ${participants_config_path[1]}
		echo -e "coordinator_info 192.168.66.101:8001" >> ${participants_config_path[1]}
		echo -e	"participant_info 192.168.66.202:8003" >> ${participants_config_path[1]}

		echo -e "mode participant" > ${participants_config_path[2]}
		echo -e	"coordinator_info 192.168.66.101:8001" >> ${participants_config_path[2]}
		echo -e	"participant_info 192.168.66.203:8004" >> ${participants_config_path[2]}
	else 
		echo -e "follower_info 192.168.66.201:8001" > ${followers_config_path[0]}
		echo -e	"follower_info 192.168.66.202:8002" >> ${followers_config_path[0]}
		echo -e	"follower_info 192.168.66.203:8003" >> ${followers_config_path[0]}

		echo -e "follower_info 192.168.66.202:8002" > ${followers_config_path[1]}
		echo -e	"follower_info 192.168.66.201:8001" >> ${followers_config_path[1]}
		echo -e	"follower_info 192.168.66.203:8003" >> ${followers_config_path[1]}

		echo -e "follower_info 192.168.66.203:8003" > ${followers_config_path[2]}
		echo -e "follower_info 192.168.66.201:8001" >> ${followers_config_path[2]}
		echo -e "follower_info 192.168.66.202:8002" >> ${followers_config_path[2]}
	fi
}

# Clear generated configuration files
function clean_up_config_files
{
	rm -f ${LAB3_ABSOLUTE_PATH}/*.conf
}

# Adding virtual network cards to the testing environment
function add_virtual_nics
{
	echo ${PASSWORD} | sudo -S ifconfig lo:0 192.168.66.101/24
	echo ${PASSWORD} | sudo -S ifconfig lo:1 192.168.66.201/24
	echo ${PASSWORD} | sudo -S ifconfig lo:2 192.168.66.202/24
	echo ${PASSWORD} | sudo -S ifconfig lo:3 192.168.66.203/24
}

# Clear virtual network cards created in the testing environment
function remove_virtual_nics
{
	echo ${PASSWORD} | sudo -S ifconfig lo:0 down
	echo ${PASSWORD} | sudo -S ifconfig lo:1 down
	echo ${PASSWORD} | sudo -S ifconfig lo:2 down
	echo ${PASSWORD} | sudo -S ifconfig lo:3 down
}

# Set the specified transmission delay for each virtual network card
function set_virtual_nics_delay
{
	echo ${PASSWORD} | sudo -S tc qdisc add dev lo:0 root netem delay ${DELAY}ms
	echo ${PASSWORD} | sudo -S tc qdisc add dev lo:1 root netem delay ${DELAY}ms
	echo ${PASSWORD} | sudo -S tc qdisc add dev lo:2 root netem delay ${DELAY}ms
	echo ${PASSWORD} | sudo -S tc qdisc add dev lo:3 root netem delay ${DELAY}ms
}

# Set the specified packet loss rate for each virtual network card
function set_virtual_nics_packet_loss
{
	echo ${PASSWORD} | sudo -S tc qdisc add dev lo:0 root netem loss ${PACKET_LOSS_RATE}%
	echo ${PASSWORD} | sudo -S tc qdisc add dev lo:1 root netem loss ${PACKET_LOSS_RATE}%
	echo ${PASSWORD} | sudo -S tc qdisc add dev lo:2 root netem loss ${PACKET_LOSS_RATE}%
	echo ${PASSWORD} | sudo -S tc qdisc add dev lo:3 root netem loss ${PACKET_LOSS_RATE}%
}

# Check if Netcat is installed
function check_netcat
{
	echo ${PASSWORD} | sudo -S apt install netcat
}

# Configure the startup function for the testing environment
function init_network_env
{
	check_netcat
	add_virtual_nics
	set_virtual_nics_delay
	set_virtual_nics_packet_loss
}

# Compile project production executable files
function do_make
{
	echo "Start make, waiting for a while......"
	make -C ${LAB3_ABSOLUTE_PATH}/
	retval=$?

	if [ $retval -eq 0 ]
	then
		echo "Make successfully"
		return $SUCCESS
	else
		echo "[Warning] : Make failed"
		return $FAIL
	fi
}

# Check if the specified process started successfully
# Parameter: [process id]
function check_background_process_start_status
{
	bp_pid=$1

	if ps | grep "$bp_pid[^[]" >/dev/null; then
    	return $SUCCESS
	else 
		return $FAIL
	fi
}

# Check what language the code is implemented in to execute different startup logic
function language_checking
{
	echo "Language checking......"
	java_file_counter=`ls -1 ${LAB3_ABSOLUTE_PATH}/*.java ${LAB3_ABSOLUTE_PATH}/*.jar 2>/dev/null | wc -l`
	jar_file_counter=`ls -1 ${LAB3_ABSOLUTE_PATH}/kvstore2pcsystem.jar 2>/dev/null | wc -l`
	c_file_counter=`ls -1 ${LAB3_ABSOLUTE_PATH}/*.c 2>/dev/null | wc -l`
	cpp_file_counter=`ls -1 ${LAB3_ABSOLUTE_PATH}/*.cc ${LAB3_ABSOLUTE_PATH}/*.cpp ${LAB3_ABSOLUTE_PATH}/*.hpp 2>/dev/null | wc -l`
	python_file_counter=`ls -1 ${LAB3_ABSOLUTE_PATH}/*.py 2>/dev/null | wc -l`
	kvstore2pcsystem_py_file_counter=`ls -1 ${LAB3_ABSOLUTE_PATH}/kvstore2pcsystem.py 2>/dev/null | wc -l`

	if [ $java_file_counter -gt 0 ]
	then
		if [ $c_file_counter -eq 0 -a $cpp_file_counter -eq 0 ] && [ $python_file_counter -eq 0 ]
		then
			echo "Language: [ JAVA ]"
			TEST_RESULT_ARR[0]="JAVA"
			if [ $jar_file_counter -eq 0 ]
			then
				echo "[Warning] : Have no file: kvstore2pcsystem.jar"
				return $NO_JAR_FILE
			fi
			return $JAVA
		fi
	fi

	if [ $python_file_counter -gt 0 ]
	then
		if [ $c_file_counter -eq 0 -a $cpp_file_counter -eq 0 ] && [ $java_file_counter -eq 0 ]
		then
			echo "Language: [ PYTHON ]"
			TEST_RESULT_ARR[0]="PYTHON"
			if [ $kvstore2pcsystem_py_file_counter -eq 0 ]
			then
				echo "Warning: [ Have no file: kvstore2pcsystem.py ]"
				return $NO_KVSTORE2PCSYSTEM_PY_FILE
			fi
			return $PYTHON
		fi
	fi

	if [ $c_file_counter -gt 0 ] || [ $cpp_file_counter -gt 0 ]
	then
		if [ $java_file_counter -eq 0 ] && [ $python_file_counter -eq 0 ]
		then
			echo "Language: [ C/C++ ]"
			TEST_RESULT_ARR[0]="C/C++"
			return $C_OR_CPP
		fi
	else
		echo "Warning: [ UNKNOWN programing_language ]"
		TEST_RESULT_ARR[0]="UNKNOWN"
		return $UNKNOWN_LANGUAGE
	fi
}

# Launch executable files in different language versions
# Parameter: [language code] [configuration file path]
function start_program
{
	case $1 in
    $C_OR_CPP)
        ${LAB3_ABSOLUTE_PATH}"/${EXECUTABLE_FILE_NAME}" --config_path $2 &
        ;;
    $JAVA)
        java -jar ${LAB3_ABSOLUTE_PATH}"/${EXECUTABLE_FILE_NAME}.jar" --config_path $2 &
        ;;
    $PYTHON)
        python3 ${LAB3_ABSOLUTE_PATH}"/${EXECUTABLE_FILE_NAME}.py" --config_path $2 &
        ;;
    *)
        ${LAB3_ABSOLUTE_PATH}"/${EXECUTABLE_FILE_NAME}" --config_path $2 &
        ;;
	esac
}

# Start the storage system robustly
# Parameter: [start mode] [language code] [configuration file index]
function run_kvstoresystem_robustly_2pc
{
	retval=$FAIL # parameter initialization

	if [[ $1 -eq $START_ONE_DATA_PROCESS_SPCECIFIC ]]; then
		check_background_process_start_status ${participants_pid[$3]}
		if [[ $? -eq $SUCCESS ]]; then
			echo "Run participant[$3] successfully"
			return $SUCCESS
		else
			for (( j=0; j<$START_RETYR_TIMES; j++ ))
			do
				start_program $2 ${participants_config_path[$3]}
				check_background_process_start_status $!
				retval=$?
				sleep 0.5

				if [[ $retval -eq $SUCCESS ]]
				then
					echo "Run participant[$3] successfully"
					participants_pid[$3]=$!
					return $SUCCESS
				else
					echo "Run participant[$3]. Retry times: [$j]"
					continue
				fi
			done

			if [[ $retval -ne $SUCCESS ]]
			then
				echo "Run participant[$3] failed"
				return $FAIL
			fi
		fi
	fi

	
	for (( i=0; i<$START_RETYR_TIMES; i++ ))
	do
		start_program $2 ${coordinator_config_path}
		check_background_process_start_status $!
		retval=$?
		sleep 1

		if [[ $retval -eq $SUCCESS ]]
		then
			echo "Run coordinator successfully"
			coordinator_pid=$!
			break
		else
			sleep 1
			continue
		fi
	done

	if [ $retval -eq $SUCCESS ]
	then
		if [[ $1 -eq $START_COORDINATOR_ONLY ]]
		then
			return $SUCCESS
		fi

		for (( i=0; i<3; i++ ))
		do
			for (( j=0; j<$START_RETYR_TIMES; j++ ))
			do
				start_program $2 ${participants_config_path[i]}
				check_background_process_start_status $!
				retval=$?

				if [[ $retval -eq $SUCCESS ]]
				then
					echo "Run participant[$i] successfully"
					participants_pid[$i]=$!
					break
				else
					echo "Run participant[$i]. Retry times: [$j]"
					continue
				fi
			done

			if [[ $retval -ne $SUCCESS ]]
			then
				echo "Run participant[$i] failed"
				return $FAIL
			fi
		done
	else
		echo "Run coordinator failed"
		return $FAIL
	fi

	echo "Run ${EXECUTABLE_FILE_NAME} successfully"
	return $SUCCESS
}

# Start the storage system robustly
# Parameter: [start mode] [language code] [configuration file index]
function run_kvstoresystem_robustly_raft
{
	retval=$FAIL # parameter initialization

	if [[ $1 -eq $START_ONE_DATA_PROCESS_SPCECIFIC ]]; then
		check_background_process_start_status ${followers_pid[$3]}
		if [[ $? -eq $SUCCESS ]]; then
			echo "Run follower[$3] successfully"
			return $SUCCESS
		else
			for (( j=0; j<$START_RETYR_TIMES; j++ ))
			do
				start_program $2 ${followers_config_path[$3]}
				check_background_process_start_status $!
				retval=$?
				sleep 0.5

				if [[ $retval -eq $SUCCESS ]]
				then
					echo "Run follower[$3] successfully"
					followers_pid[$3]=$!
					return $SUCCESS
				else
					echo "Run follower[$3]. Retry times: [$j]"
					continue
				fi
			done

			if [[ $retval -ne $SUCCESS ]]
			then
				echo "Run follower[$3] failed"
				return $FAIL
			fi
		fi
	fi

	for (( i=0; i<3; i++ ))
	do
		for (( j=0; j<$START_RETYR_TIMES; j++ ))
		do
			start_program $2 ${followers_config_path[i]}
			check_background_process_start_status $!
			retval=$?

			if [[ $retval -eq $SUCCESS ]]
			then
				echo "Run follower[$i] successfully"
				followers_pid[$i]=$!
				break
			else
				echo "Run follower[$i]. Retry times: [$j]"
				continue
			fi
		done

		if [[ $retval -ne $SUCCESS ]]
		then
			echo "Run follower[$i] failed"
			return $FAIL
		fi
	done

	echo "Run ${EXECUTABLE_FILE_NAME} successfully"
	return $SUCCESS
}

# Start the storage system robustly
# Parameter: [start mode] [configuration file index]
function run_kvstoresystem_robustly
{
	retval=$FAIL # parameter initialization

	language_checking

	programing_language=$?

	# Do make if there's a makefile regardless of in what language the system is written.
	if [ -f ${LAB3_ABSOLUTE_PATH}/Makefile ] || [ -f ${LAB3_ABSOLUTE_PATH}/makefile ]
	then
		do_make
		if [ $? -eq $FAIL ]; then
			return $FAIL
		fi
	fi

	if [ $VERSION -eq 1 ]; then
		run_kvstoresystem_robustly_2pc $1 $programing_language $2
		return $?
	else
		run_kvstoresystem_robustly_raft $1 $programing_language $2
		return $?
	fi
}

# Kill the coordinator process and restart
function kill_and_restart_coordinator_robustly
{
	echo "Kill coordinator and then restart."

	kill -9 ${coordinator_pid}
	sleep 1
	run_kvstoresystem_robustly $START_COORDINATOR_ONLY

	retval=$?
	if [[ $retval -eq $SUCCESS ]]
	then
		return $SUCCESS
	else
		return $FAIL
	fi
}

# Kill the participant processes and coordinator process then restart
function kill_coordinator_and_all_participants
{
	kill -9 ${coordinator_pid}

	for (( i=0; i<3; i++ ))
	do
		kill -9 ${participants_pid[i]}
	done
}

function kill_coordinator
{
	kill -9 ${coordinator_pid}
}

function kill_one_of_participants
{
	kill -9 ${participants_pid[0]}
}

function kill_two_of_participants
{
	for (( i=1; i<3; i++ ))
	do
		kill -9 ${participants_pid[i]}
	done	
}

function kill_all_participants
{
	for (( i=0; i<3; i++ ))
	do
		kill -9 ${participants_pid[i]}
	done
}

function kill_one_of_followers
{
	kill -9 ${followers_pid[0]}
}

function kill_two_of_followers
{
	for (( i=1; i<3; i++ ))
	do
		kill -9 ${followers_pid[i]}
	done	
}

function kill_all_of_followers
{
	for (( i=0; i<3; i++ ))
	do
		kill -9 ${followers_pid[i]}
	done	
}

function kill_leader
{
	index=${LEADER_PORT: -1}
	kill -9 ${followers_pid[$index]}
}

# Restart the suspended service
function restart_kvstoresystem_if_down_abnormally
{
	if [ $VERSION -eq 1 ]; then
		if ! ps -p $coordinator_pid > /dev/null
		then
			kill_coordinator_and_all_participants
			sleep 1
			run_kvstoresystem_robustly $START_COORDINATOR_AND_ALL_PARTICIPANTS
			return
		fi

		for (( i=0; i<3; i++ ))
		do
			if ! ps -p ${participants_pid[i]} > /dev/null
			then
				run_kvstoresystem_robustly $START_ONE_DATA_PROCESS_SPCECIFIC $i
				sleep 1
			fi
		done
	else 
		for (( i=0; i<3; i++ ))
		do
			if ! ps -p ${followers_pid[i]} > /dev/null
			then
				run_kvstoresystem_robustly $START_ONE_DATA_PROCESS_SPCECIFIC $i
				sleep 1
			fi
		done
	fi
}

function parse_leader_infomation
{
	LEADER_IP=${1%%:*}
	LEADER_PORT=${1#*:}
}

function send_set_command
{
	key_len=$1
	key=$2
	value_len="$3"
	value=$4

	printf -v set_command "*3\r\n\$3\r\nSET\r\n\$${key_len}\r\n${key}\r\n\$${value_len}\r\n${value}\r\n"

	for (( i=0; i<$ERROR_RETRY_TIMES; i++ ))
	do
		if [ $VERSION -eq 1 ]; then
			retval_set=`printf "$set_command" | nc -w ${NC_TIMEOUT} ${COORDINATOR_IP} ${COORDINATOR_PORT}`
		else 
			retval_set=`printf "$set_command" | nc -w ${NC_TIMEOUT} ${LEADER_IP} ${LEADER_PORT}`
			if [ $? -ne 0 ]; then
				LEADER_IP=${follower_ip[i]}
				LEADER_PORT=${follower_port[i]}
				continue
			fi
			if [[ $retval_set =~ $leader_infomation ]]; then
				parse_leader_infomation $retval_set
				continue
			fi
		fi	

	    if [[ $retval_set =~ $standard_error ]]
	    then
	    	sleep 0.5
	    	continue
	    else
	    	break
	    fi
	done

	printf -v set_result "%s" "${retval_set}"
}

function send_get_command
{
	key_len=$1
	key=$2

	printf -v get_command "*2\r\n\$3\r\nGET\r\n\$${key_len}\r\n${key}\r\n"
	for (( i=0; i<$ERROR_RETRY_TIMES; i++ ))
	do
		if [ $VERSION -eq 1 ]; then
			retval_get=`printf "$get_command" | nc -w ${NC_TIMEOUT} ${COORDINATOR_IP} ${COORDINATOR_PORT}`
		else 
			retval_get=`printf "$get_command" | nc -w ${NC_TIMEOUT} ${LEADER_IP} ${LEADER_PORT}`
			if [ $? -ne 0 ]; then
				LEADER_IP=${follower_ip[i]}
				LEADER_PORT=${follower_port[i]}
				continue
			fi
			if [[ $retval_get =~ $leader_infomation ]]; then
				parse_leader_infomation $retval_get
				continue
			fi
		fi

	    if [[ $retval_get =~ $standard_error ]]
	    then
	    	sleep 0.5
	    	continue
	    else
	    	break
	    fi
	done

	printf -v get_result "%s" "$retval_get"
}

printf -v del_standard_return ":(.*)\r"
del_1_result=""
function send_del_command_1
{
	key1_len=$1
	key1=$2

	printf -v del_command_1 "*2\r\n\$3\r\nDEL\r\n\$${key1_len}\r\n${key1}\r\n"
	for (( i=0; i<$ERROR_RETRY_TIMES; i++ ))
	do
		if [ $VERSION -eq 1 ]; then
			retval_del1=`printf "$del_command_1" | nc -w ${NC_TIMEOUT} ${COORDINATOR_IP} ${COORDINATOR_PORT}`
		else
			retval_del1=`printf "$del_command_1" | nc -w ${NC_TIMEOUT} ${LEADER_IP} ${LEADER_PORT}`
		fi

		if [[ $retval_del1 =~ $del_standard_return ]]; then
			break
		fi
		if [[ $retval_del1 =~ $standard_error ]]
		then
			sleep 0.5
			continue
		fi
		if [ $VERSION -eq 2 ]; then
			if [[ $retval_del1 =~ $leader_infomation ]]; then
				parse_leader_infomation $retval_del1
				continue
			fi
			LEADER_IP=${follower_ip[i]}
			LEADER_PORT=${follower_port[i]}
		fi
	done

	printf -v del_1_result "%s" "$retval_del1"
}

del_2_result=""
function send_del_command_2
{
	key1_len=$1
	key1=$2
	key2_len=$3
	key2=$4
	printf -v del_command_2 "*3\r\n\$3\r\nDEL\r\n\$${key1_len}\r\n${key1}\r\n\$${key2_len}\r\n${key2}\r\n"
	for (( i=0; i<$ERROR_RETRY_TIMES; i++ ))
	do
		if [ $VERSION -eq 1 ]; then
			retval_del2=`printf "$del_command_2" | nc -w ${NC_TIMEOUT} ${COORDINATOR_IP} ${COORDINATOR_PORT}`
		else
			retval_del2=`printf "$del_command_2" | nc -w ${NC_TIMEOUT} ${LEADER_IP} ${LEADER_PORT}`
		fi 

		if [[ $retval_del2 =~ $del_standard_return ]]; then
			break
		fi
		if [[ $retval_del2 =~ $standard_error ]]
	    then
			sleep 0.5
			continue
		fi
		if [ $VERSION -eq 2 ]; then
			if [[ $retval_del2 =~ $leader_infomation ]]; then
				parse_leader_infomation $retval_del2
				continue
			fi
			LEADER_IP=${follower_ip[i]}
			LEADER_PORT=${follower_port[i]}
		fi
	done

	printf -v del_2_result "%s" "$retval_del2"
}


# ######################## basic version lv1 ########################

function set_tag
{
	echo "                                       | |                                   "
	echo "                                       | |                                   "
	echo "                                       \|/                                   "
}

printf -v standard_error "%s\r" "-ERROR"
printf -v standard_ok "+OK\r"
printf -v standard_nil "*1\r\n\$3\r\nnil\r"

standard_item1=""
function test_item1
{
	set_tag
	echo "------------------------------------- Test item 1 -------------------------------------"
	echo "Test item 1. Test point: Run kvstore2pcsystem."

	run_kvstoresystem_robustly $START_COORDINATOR_AND_ALL_PARTICIPANTS

	sleep 5

	retval=$?
	if [[ $retval -eq $SUCCESS ]]
	then
		echo -e "============================ [\e[32mPASSED\e[0m] : Test item 1 ============================"
		return $PASSED
	else
		echo -e "============================ [\e[31mFAILED\e[0m] : Test item 1 ============================"
		return $FAILED
	fi
}

standard_item2="$standard_ok"
function test_item2
{
	set_tag
	echo "------------------------------------- Test item 2 -------------------------------------"
	echo "Test item 2. Test point: Set key to hold string value."

	send_set_command 9 item2_key 11 item2_value

	echo "item set set_result: ${set_result}"
	if [[ $set_result = $standard_item2 ]]
	then
		echo -e "============================ [\e[32mPASSED\e[0m] : Test item 2 ============================"
		return $PASSED
	else
		echo -e "============================ [\e[31mFAILED\e[0m] : Test item 2 ============================"
		return $FAILED
	fi
}


printf -v standard_item3 "*1\r\n\$11\r\nitem3_value\r"
function test_item3
{
	set_tag
	echo "------------------------------------- Test item 3 -------------------------------------"
	echo "Test item 3. Test point: Get the value of key."

	send_set_command 9 item3_key 11 item3_value
	sleep 3
	if [ $VERSION -eq 1 ]; then
		kill_and_restart_coordinator_robustly
	else 
		kill_leader
		sleep 5
		restart_kvstoresystem_if_down_abnormally
		sleep 5
	fi
	send_get_command 9 item3_key
	echo "get_result: ${get_result}"
	if [[ $get_result = $standard_item3 ]]
	then
		echo -e "============================ [\e[32mPASSED\e[0m] : Test item 3 ============================"
		return $PASSED
	else
		echo -e "============================ [\e[31mFAILED\e[0m] : Test item 3 ============================"
		return $FAILED
	fi
}


standard_item4="$standard_nil"
function test_item4
{
	set_tag
	echo "------------------------------------- Test item 4 -------------------------------------"
	echo "Test item 4. Test point: Return nil if the key does no exist."

	send_get_command 9 item4_key

	if [[ $get_result = $standard_item4 ]]
	then
		echo -e "============================ [\e[32mPASSED\e[0m] : Test item 4 ============================"
		return $PASSED
	else
		echo -e "============================ [\e[31mFAILED\e[0m] : Test item 4 ============================"
		return $FAILED
	fi

}


printf -v standard_item5 ":2\r"
function test_item5
{
	set_tag
	echo "------------------------------------ Test item 5 -------------------------------------"
	echo "Test item 5. Test point: If the DEL command executed, return the number of keys that were removed."

	send_set_command 11 item5_key_1 13 item5_value_1

	sleep 1

	send_set_command 11 item5_key_2 13 item5_value_2

	sleep 1
	send_del_command_2 11 item5_key_1 11 item5_key_2

	sleep 1

	if [[ $del_2_result = $standard_item5 ]]
	then
		echo -e "============================ [\e[32mPASSED\e[0m] : Test item 5 ============================"
		return $PASSED
	else
		echo -e "============================ [\e[31mFAILED\e[0m] : Test item 5 ============================"
		return $FAILED
	fi
}


printf -v standard_item6 "*1\r\n\$15\r\nitem6_value_new\r"
function test_item6
{
	set_tag
	echo "------------------------------------- Test item 6 -------------------------------------"
	echo "Test item 6. Test point: When associating a new value to an existing key," \
	     " it should overwrite the value of the existing entry,"

	send_set_command 9 item6_key 11 item6_value

	sleep 1
	send_set_command 9 item6_key 15 item6_value_new

	sleep 1

	send_get_command 9 item6_key

	sleep 1

	if [[ $get_result = $standard_item6 ]]
	then
		echo -e "============================ [\e[32mPASSED\e[0m] : Test item 6 ============================"
		return $PASSED
	else
		echo -e "============================ [\e[31mFAILED\e[0m] : Test item 6 ============================"
		return $FAILED
	fi
}


standard_item7="$standard_nil"
function test_item7
{
	set_tag
	echo "------------------------------------- Test item 7 -------------------------------------"
	echo "Test item 7. Test point: Correctness testing of DEL command."

	send_set_command 9 item7_key 11 item7_value

	sleep 1

	send_del_command_1 9 item7_key

	sleep 1

	send_get_command 9 item7_key

	sleep 1

	if [[ $get_result = $standard_item7 ]]
	then
		echo -e "============================ [\e[32mPASSED\e[0m] : Test item 7 ============================"
		return $PASSED
	else
		echo -e "============================ [\e[31mFAILED\e[0m] : Test item 7 ============================"
		return $FAILED
	fi
}


########################## basic version lv2 ########################

printf -v standard_item8 "*1\r\n\$17\r\nitem8_key_value_3\r"
function test_item8
{
	set_tag
	echo "------------------------------------- Test item 8 -------------------------------------"
	echo "Test item 8. Test point: Kill one of the store processes."

	send_set_command 9 item8_key 17 item8_key_value_1

	sleep 1

	send_set_command 9 item8_key 17 item8_key_value_2

	sleep 1
	if [ $VERSION -eq 1 ]; then
		kill_one_of_participants
		sleep 2
	else 
		kill_one_of_followers
		sleep 5
	fi
	send_set_command 9 item8_key 17 item8_key_value_3

	sleep 1

	send_get_command 9 item8_key

	sleep 1

	if [[ $get_result = $standard_item8 ]]
	then
		echo -e "============================ [\e[32mPASSED\e[0m] : Test item 8 ============================"
		return $PASSED
	else
		echo -e "============================ [\e[31mFAILED\e[0m] : Test item 8 ============================"
		return $FAILED
	fi

}


standard_item9="$standard_error"
function test_item9
{
	set_tag
	echo "------------------------------------- Test item 9 -------------------------------------"
	echo "Test item 9. Test point: Kill all storage proccesses."

	send_set_command 9 item9_key 17 item9_key_value_1

	sleep 1
	send_set_command 9 item9_key 17 item9_key_value_2

	sleep 1
	if [ $VERSION -eq 1 ]; then
		kill_all_participants
	else 
		return $PASSED
	fi
	

	send_get_command 9 item9_key

	if [[ $get_result =~ $standard_item9 ]]
	then
		echo -e "============================ [\e[32mPASSED\e[0m] : Test item 9 ============================"
		return $PASSED
	else
		echo -e "============================ [\e[31mFAILED\e[0m] : Test item 9 ============================"
		return $FAILED
	fi

}

########################## basic version lv3 ########################

printf -v standard_get_item10 "*1\r\n\$20\r\nitem10_key_2_value_2\r"
printf -v standard_del_item10 ":2\r"
function test_item10
{
	set_tag
	echo "------------------------------------ Test item 10 -------------------------------------"
	echo "Test item 10. Test point: extreme version test, Kill two storage nodes and restart them before testing GET and DEL."

	restart_kvstoresystem_if_down_abnormally

	send_set_command 12 item10_key_1 20 item10_key_1_value_1

	sleep 1
	send_set_command 12 item10_key_2 20 item10_key_2_value_2

	sleep 1

	if [ $VERSION -eq 1 ]; then
		kill_two_of_participants
	else	
		kill_two_of_followers
	fi
	
	sleep 5

	restart_kvstoresystem_if_down_abnormally
	
	sleep 10

	send_get_command 12 item10_key_2

	sleep 1

	get_success=0

	if [[ $get_result = $standard_get_item10 ]]
	then
		get_success=1
	fi

	send_del_command_2 12 item10_key_1 12 item10_key_2
	
	if [ $del_2_result = $standard_del_item10 ] && [ $get_success -eq 1 ]
	then
		echo -e "============================ [\e[32mPASSED\e[0m] : Test item 10 ============================"
		return $PASSED
	else
		echo -e "============================ [\e[31mFAILED\e[0m] : Test item 10 ============================"
		return $FAILED
	fi
}

# Start all test points
function cloud_roll_up
{
	echo "---------------------------------- Global test start ----------------------------------"
	test_item1
	TEST_RESULT_ARR[1]=$?
	if [[ ${TEST_RESULT_ARR[1]} -eq $FAILED ]]
	then
		echo "---------------------------------- Start system failed. Global test done ----------------------------------"
		return
	fi
	test_item2
	TEST_RESULT_ARR[2]=$?
	test_item3
	TEST_RESULT_ARR[3]=$?
	test_item4
	TEST_RESULT_ARR[4]=$?
	test_item5
	TEST_RESULT_ARR[5]=$?
	test_item6
	TEST_RESULT_ARR[6]=$?
	test_item7
	TEST_RESULT_ARR[7]=$?
	test_item8
	TEST_RESULT_ARR[8]=$?
	test_item9
	TEST_RESULT_ARR[9]=$?
	test_item10
	TEST_RESULT_ARR[10]=$?

	echo "---------------------------------- Global test done -----------------------------------"
}

# Clear testing environment
function clean_up
{
	if [ $VERSION -eq 1 ]; then
		kill_coordinator_and_all_participants
	else
		kill_all_of_followers
	fi
	remove_virtual_nics
	clean_up_config_files
}

# Show test results
function show_test_result
{
	echo "Language: [ ${TEST_RESULT_ARR[0]} ]"
	echo "VERSION: [ ${VERSION} ]"
	echo -n > $LAB3_TEST_RESULT_SAVE_PATH/lab3_test_result.csv
	echo "---------------------------------- Passing situation ----------------------------------"
	for (( i=1; i<11; i++ ))
	do
		if [[ ${TEST_RESULT_ARR[i]} -eq $PASSED ]]
		then
			echo -e "Test items ${i} [ \e[32mPASSED\e[0m ]"
			echo -e "PASSED \c" >> $LAB3_TEST_RESULT_SAVE_PATH/lab3_test_result.csv
		else
			echo -e "Test items ${i} [ \e[31mFAILED\e[0m ]"
			echo -e "FAILED \c" >> $LAB3_TEST_RESULT_SAVE_PATH/lab3_test_result.csv
		fi
	done

	# The logic of calculating scores: for the first 7 tests, each item receives 2 points. 
	# If all 7 items pass, 15 points will be given, the last 3 items receive 1 point per item;.
	# If the final total score is 18 points and the implemented version is advanced version, 20 points will be given.
	total_score=0
	for (( i=1; i<8; i++ ))
	do
		if [[ ${TEST_RESULT_ARR[i]} -eq $PASSED ]]
		then
			total_score=`expr $total_score + 2`
		fi
	done

	if [[ $total_score -eq 14 ]]
	then
		total_score=15
	fi

	for (( i=8; i<11; i++ ))
	do
		if [[ ${TEST_RESULT_ARR[i]} -eq $PASSED ]]
		then
			total_score=`expr $total_score + 1`
		fi
	done

	if [ $VERSION -eq 2 ]
	then
		total_score=`expr $total_score + 2`
	fi

	echo -e  "--------------------------------- Total score: [ \e[33m${total_score}\e[0m ] ----------------------------------"
}

# Prepare testing environment
function prepare_test_env
{
	set_executable_file_name
	generate_config_files
	init_network_env
}

############################### End of testing using function definitions ##################################

############################### Main function execution for testing ########################################

prepare_test_env
cloud_roll_up
clean_up
show_test_result