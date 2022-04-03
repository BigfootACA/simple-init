function ewrite(){
	[ -n "${1}" ]||return 1
	[ -n "${2}" ]||return 1
	local FILE="${1}"
	shift
	[ -e "${FILE}" ]||echo "$*" > "${FILE}"
}
function eln(){
	[ -n "${1}" ]||return 1
	[ -n "${2}" ]||return 1
	[ -e "${2}" ]||ln -s "${1}" "${2}"
}
function emkdir(){
	[ -n "${1}" ]||return 1
	for i in "${@}"
	do [ -e "${i}" ]||mkdir -m0755 "${i}"
	done
}
function add_library(){
	local NAME FILE
	[ -f "${1}" ]||return 1
	[ -n "${2}" ]||return 1
	NAME="$(basename "${1}")"
	FILE="${2}/lib/${NAME}"
	if ! [ -f "${FILE}" ]
	then	echo "add library ${NAME}"
		if cp "${1}" "${FILE}"
		then	chmod 0755 "${FILE}"
			strip "${FILE}"||true
		fi
	fi
	unset FILE NAME
}
function add_depend(){
	[ -f "${1}" ]||return 1
	[ -n "${2}" ]||return 1
	add_library "$(readelf -l "${1}"|awk '/Requesting program interpreter/{print substr($4,0,length($4)-1)}')" "${2}"||true
	for i in $(ldd "${1}" 2>/dev/null|awk '{print $3}'|grep '^/'|sort|uniq)
	do add_library "${i}" "${2}"
	done
	unset i
}
function add_binary(){
	local NAME FILE
	[ -f "${1}" ]||return 1
	[ -n "${2}" ]||return 1
	NAME="$(basename "${1}")"
	FILE="${2}/bin/${NAME}"
	echo "add binary ${NAME}"
	if ! cp "${1}" "${FILE}"
	then	unset FILE NAME
		return 1
	fi
	chmod 0755 "${FILE}"
	strip "${FILE}"
	add_depend "${1}" "${2}"
	unset FILE NAME
}
function add_cmd(){
	local FILE
	[ -n "${1}" ]||return 1
	[ -n "${2}" ]||return 1
	if ! FILE="$(which "${1}" 2>/dev/null)"
	then	echo "Program ${1} not found"
		unset FILE
		return 1
	fi
	add_binary "${FILE}" "${2}"
	unset FILE
}
function add_lib(){
	local FILE TARGET
	[ -n "${1}" ]||return 1
	[ -n "${2}" ]||return 1
	TARGET="$(gcc -dumpmachine)"
	for i in \
		/lib /lib64 /usr/lib /usr/lib64 \
		/usr/local/lib /usr/local/lib64 \
		/lib/"${TARGET}" /lib64/"${TARGET}" \
		/usr/lib/"${TARGET}" /usr/lib64/"${TARGET}"
	do	x="${i}/${1}"
		[ -f "${x}" ]||continue
		FILE="${x}"
		unset x
		break
	done
	unset i
	if ! [ -f "${FILE}" ]
	then	echo "Library ${1} not found"
		unset FILE
		return 1
	fi
	add_library "${FILE}" "${2}"
	add_depend "${FILE}" "${2}"
	unset FILE
}
function add_data(){
	local TARGET
	[ -e "/${1}" ]||return 1
	[ -n "${2}" ]||return 1
	TARGET="${2}/${1}"
	if ! [ -e "${TARGET}" ]
	then	echo "add data ${1}"
		mkdir -p "$(dirname "${TARGET}")"
		cp -r "/${1}" "${TARGET}"
	fi
	unset TARGET
}
function init_busybox(){
	local LIST
	if [ "${USER}" == "root" ]
	then LIST="$(chroot "${1}" busybox --list)"
	else LIST="$(busybox --list)"
	fi
	for i in ${LIST}
	do eln busybox "${1}/usr/bin/$i"
	done
	unset LIST
}
function init_rootfs(){
	[ -d "${1}" ]||return 1
	emkdir "${1}"/{usr,opt,srv,etc,proc,sys,dev,var,tmp,run,home,root}
	emkdir "${1}"/usr/{bin,lib,share}
	emkdir "${1}"/var/{lib,log,cache}
	eln ../run "${1}"/var/run
	eln . "${1}"/usr/lib/x86_64-linux-gnu
	eln usr/lib "${1}"/lib
	eln usr/lib "${1}"/lib64
	eln usr/bin "${1}"/bin
	eln usr/bin "${1}"/sbin
	eln bin "${1}"/usr/sbin
	eln lib "${1}"/usr/lib64
	ewrite "${1}"/etc/passwd 'root:x:0:0::/root:/bin/bash'
	ewrite "${1}"/etc/group 'root:x:0:root'
	ewrite "${1}"/etc/shadow 'root::::::::'
	chmod 0644 "${1}"/etc/passwd
	chmod 0644 "${1}"/etc/group
	chmod 0600 "${1}"/etc/shadow
	chmod 0555 "${1}"/sys
	chmod 1777 "${1}"/tmp
	if [ "${USER}" == "root" ]
	then	mknod -m0666 "${1}"/dev/null c 1 3||true
		mknod -m0600 "${1}"/dev/console c 5 1||true
	fi
}
function gen_initcpio(){
	local _INITCPIO
	[ -d "${1}" ]||return 1
	_INITCPIO="$(realpath "${2}")"
	pushd "${1}" >/dev/null||return 1
	if ! [ -x init ]
	then	echo "No init found"
		popd >/dev/null||true
		return 1
	fi
	find .|sort|cpio \
		--quiet \
		--create \
		--owner=root:root \
		--format=newc\
		|${COMPRESS} >"${_INITCPIO}"
	popd >/dev/null||return 1
}
