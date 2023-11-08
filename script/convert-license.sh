#!/bin/bash
# @file convert-license.sh
# @author Daniel Starke
# @date 2019-09-30
# @version 2019-09-30

INPUT="${1}"
OUTPUT="${2}"

export LANG="C"
export LANGUAGE="C"
export LC_ALL="C"

if [ ! -e "${INPUT}" ]; then
	echo "Error: Input file not found \"${INPUT}\"." >&2
	exit 1
fi

if [ ! -d "`dirname ${OUTPUT}`" ]; then
	echo "Error: Output directory not found \"`dirname ${OUTPUT}`\"." >&2
	exit 1
fi

if ! touch "${OUTPUT}"; then
	echo "Error: Failed to create output file \"${OUTPUT}\"." >&2
	exit 1
fi

oldIFS="${IFS}"
IFS=""
{
	cat << _FILE_HEAD
/**
 * @file `basename ${OUTPUT}`
 * @author ${vkvm_author}
 * @copyright Copyright `date +"%Y"` ${vkvm_author}
 * @date `date +"%Y-%m-%d"`
 */

/**
 * Automatically converted license text.
 */
static const char licenseText[] = {
_FILE_HEAD
	index=0
	printf "\t"
	while read -n 1 byte
	do
		byte=$(printf "%i" "'${byte}")
		# wrong interpreted line-feed correction
		if [ "${byte}" = 0 ]; then
			byte="10"
		fi
		printf "%i," "${byte}"
		let "index++"
		if [ "${index}" -ge 32 ]; then
			printf "\n\t"
			index=0
		else
			printf " "
		fi
	done << _FILE_CONTENT
`cat "${INPUT}"`
_FILE_CONTENT
	printf "0\n};\n"
} > "${OUTPUT}"
IFS="${oldIFS}"
