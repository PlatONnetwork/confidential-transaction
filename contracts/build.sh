#!/bin/bash

# If you want to support the automatic generation of the java packaging class of the contract, please set the environment variable WEB3J in the current execution environment
# For example: export WEB3J=${WEB3JPATH}/platon-web3j

exist_optimizer=$(whereis wasm-opt | xargs | awk -F ":" '{print $2}')
exist_compiler=$(whereis platon-cpp | xargs | awk -F ":" '{print $2}')

if [ "${exist_optimizer}" == "" ] || [ "${exist_compiler}" == "" ]; then
	echo "compiler or optimizer is not exist"
	exit 1
fi

if [ ! -d build ]; then
	mkdir build
fi
OPT=-UNDEBUG
cd build
all=(storage/storage.cpp storage/confidential_storage.cpp token/arc20.cpp token/token_manager.cpp acl/acl.cpp token/confidential_token.cpp validator/plaintext_validator.cpp validator/confidential_validator.cpp multisig/multisig.cpp registry/registry.cpp)

if [ "$1" == "" ]; then
	for obj in ${all[@]}; do
		echo "compile ${obj}"
		platon-cpp ${OPT} -I ../include ../src/${obj}
		if [ 0 -ne $? ]; then
			echo "compile ${obj} failed!!!"
			exit 1
		fi
	done
else
	echo "compile $1"
	platon-cpp ${OPT} -I ../include ../src/$1
	if [ 0 -ne $? ]; then
		echo "compile $1 failed!!!"
		exit 1
	fi
fi

# Generate java wrapper class
if [ "$WEB3J" ]; then
	REALNAME=""
	function getWasmName() {
		sub_info=$(echo $1 | awk -F '/' '{for(i=1;i<=NF;i++){print $i}}')
		for one_info in $sub_info; do
			result=$(echo ${one_info} | grep ".cpp")
			if [ "$result" ]; then
				REALNAME=$(echo $result | awk -F '.' '{print $1}')
			fi
		done
	}

	all=(storage confidential_storage arc20 token_manager acl confidential_token plaintext_validator confidential_validator multisig registry)
	if [ "$1" == "" ]; then
		for obj in ${all[@]}; do
			echo "Generate java wrapper class for ${obj}"
			${WEB3J} wasm generate ${obj}.wasm ${obj}.abi.json -o . -p com.platon.privacy.contracts
			if [ 0 -ne $? ]; then
				echo "Generate java wrapper class for ${obj} failed!!!"
				exit 1
			fi
		done
	else
		getWasmName $1
		echo "Generate java wrapper class for ${REALNAME}"
		${WEB3J} wasm generate ${REALNAME}.wasm ${REALNAME}.abi.json -o . -p com.platon.privacy.contracts
	fi

	# move java wrapper class
	mv ./com/platon/privacy/contracts/*.java ../../java/privacy/privacy/src/main/java/com/platon/privacy/contracts/
fi
