#!/bin/sh
#
# link vmlinux
#
# vmlinux is linked from the objects selected by $(KBUILD_VMLINUX_OBJS) and
# $(KBUILD_VMLINUX_LIBS). Most are built-in.a files from top-level directories
# in the kernel tree, others are specified in arch/$(ARCH)/Makefile.
# $(KBUILD_VMLINUX_LIBS) are archives which are linked conditionally
# (not within --whole-archive), and do not require symbol indexes added.
#
# vmlinux
#   ^
#   |
#   +--< $(KBUILD_VMLINUX_OBJS)
#   |    +--< init/built-in.a drivers/built-in.a mm/built-in.a + more
#   |
#   +--< $(KBUILD_VMLINUX_LIBS)
#   |    +--< lib/lib.a + more
#   |
#   +-< ${kallsymso} (see description in KALLSYMS section)
#
# vmlinux version (uname -v) cannot be updated during normal
# descending-into-subdirs phase since we do not yet know if we need to
# update vmlinux.
# Therefore this step is delayed until just before final link of vmlinux.
#
# System.map is generated to document addresses of all kernel symbols

# Error out on error
set -e

# Nice output in kbuild format
# Will be supressed by "make -s"
info()
{
	if [ "${quiet}" != "silent_" ]; then
		printf "  %-7s %s\n" ${1} ${2}
	fi
}

# Thin archive build here makes a final archive with symbol table and indexes
# from vmlinux objects INIT and MAIN, which can be used as input to linker.
# KBUILD_VMLINUX_LIBS archives should already have symbol table and indexes
# added.
#
# Traditional incremental style of link does not require this step
#
# built-in.a output file
#
archive_builtin()
{
	info AR built-in.a
	rm -f built-in.a;
	${AR} cDPrsT built-in.a	 ${KBUILD_VMLINUX_OBJS}

	# rebuild with llvm-ar to update the symbol table
	if [ -n "${CONFIG_LTO_CLANG}" ]; then
		mv -f built-in.a built-in.a.tmp
		${LLVM_AR} cDrsT built-in.a $(${AR} t built-in.a.tmp)
		rm -f built-in.a.tmp
	fi
}

# If CONFIG_LTO_CLANG is selected, generate a linker script to ensure correct
# ordering of initcalls, and with CONFIG_MODVERSIONS also enabled, collect the
# previously generated symbol versions into the same script.
lto_lds()
{
	if [ -z "${CONFIG_LTO_CLANG}" ]; then
		return
	fi

	${srctree}/scripts/generate_initcall_order.pl \
		built-in.a ${KBUILD_VMLINUX_LIBS} \
		> .tmp_lto.lds

	if [ -n "${CONFIG_MODVERSIONS}" ]; then
		for a in built-in.a ${KBUILD_VMLINUX_LIBS}; do
			for o in $(${AR} t $a); do
				if [ -f ${o}.symversions ]; then
					cat ${o}.symversions >> .tmp_lto.lds
				fi
			done
		done
	fi

	echo "-T .tmp_lto.lds"
}

# We can't use --no-whole-archive flag with GCC LTO
no_whole_archive_check()
{
	if [ -n "${CONFIG_LTO_GCC}" ]; then
		NO_WHOLE_ARCHIVE=""
	else
		NO_WHOLE_ARCHIVE="--no-whole-archive"
	fi
}

# Link of vmlinux.o used for section mismatch analysis
# ${1} output file
modpost_link()
{
	local objects

	objects="--whole-archive				\
		built-in.a					\
		${NO_WHOLE_ARCHIVE}				\
		--start-group					\
		${KBUILD_VMLINUX_LIBS}				\
		--end-group"

	if [ -n "${CONFIG_LTO}" ]; then
		# This might take a while, so indicate that we're doing
		# an LTO link
		info LTO vmlinux.o
	else
		info LD vmlinux.o
	fi

	${LDFINAL} ${LDFLAGS} -r -o ${1} $(lto_lds) ${objects}
}

# If CONFIG_LTO_CLANG is selected, we postpone running recordmcount until
# we have compiled LLVM IR to an object file.
recordmcount()
{
	if [ -z "${CONFIG_LTO_CLANG}" ]; then
		return
	fi

	if [ -n "${CONFIG_FTRACE_MCOUNT_RECORD}" ]; then
		scripts/recordmcount ${RECORDMCOUNT_FLAGS} $*
	fi
}

# Link of vmlinux
# ${1} - optional extra .o files
# ${2} - output file
vmlinux_link()
{
	local lds="${objtree}/${KBUILD_LDS}"
	local objects

	if [ "${SRCARCH}" != "um" ]; then
		local ld=${LDFINAL}
		local ldflags="${LDFLAGS} ${LDFLAGS_vmlinux}"

		if [ -n "${LDFINAL_vmlinux}" ]; then
			ld=${LDFINAL_vmlinux}
			ldflags="${LDFLAGS_FINAL_vmlinux} ${LDFLAGS_vmlinux}"
		fi

		if [ -z "${CONFIG_LTO_CLANG}" ]; then
			objects="--whole-archive		\
				built-in.a			\
				${NO_WHOLE_ARCHIVE}		\
				--start-group			\
				${KBUILD_VMLINUX_LIBS}		\
				--end-group			\
				${1}"
		else
			objects="--start-group			\
				vmlinux.o			\
				--end-group			\
				${1}"
		fi

		${ld} ${ldflags} -o ${2} -T ${lds} ${objects}
	else
		objects="-Wl,--whole-archive			\
			built-in.a				\
			-Wl,${NO_WHOLE_ARCHIVE}			\
			-Wl,--start-group			\
			${KBUILD_VMLINUX_LIBS}			\
			-Wl,--end-group				\
			${1}"

		${CC} ${CFLAGS_vmlinux} -o ${2}			\
			-Wl,-T,${lds}				\
			${objects}				\
			-lutil -lrt -lpthread
		rm -f linux
	fi
}

# Create ${2} .o file with all symbols from the ${1} object file
kallsyms()
{
	info KSYM ${2}
	local kallsymopt;

	if [ -n "${CONFIG_HAVE_UNDERSCORE_SYMBOL_PREFIX}" ]; then
		kallsymopt="${kallsymopt} --symbol-prefix=_"
	fi

	if [ -n "${CONFIG_KALLSYMS_ALL}" ]; then
		kallsymopt="${kallsymopt} --all-symbols"
	fi

	if [ -n "${CONFIG_ARM}" ] && [ -z "${CONFIG_XIP_KERNEL}" ] && [ -n "${CONFIG_PAGE_OFFSET}" ]; then
		kallsymopt="${kallsymopt} --page-offset=$CONFIG_PAGE_OFFSET"
	fi

	if [ -n "${CONFIG_X86_64}" ]; then
		kallsymopt="${kallsymopt} --absolute-percpu"
	fi

	local aflags="${KBUILD_AFLAGS} ${KBUILD_AFLAGS_KERNEL}               \
		      ${NOSTDINC_FLAGS} ${LINUXINCLUDE} ${KBUILD_CPPFLAGS}"

	${NM} -n ${1} | \
		scripts/kallsyms ${kallsymopt} | \
		${CC} ${aflags} -c -o ${2} -x assembler-with-cpp -
}

# Create map file with all symbols from ${1}
# See mksymap for additional details
mksysmap()
{
	${CONFIG_SHELL} "${srctree}/scripts/mksysmap" ${1} ${2}
}

sortextable()
{
	${objtree}/scripts/sortextable ${1}
}

# Delete output files in case of error
cleanup()
{
	rm -f .tmp_System.map
	rm -f .tmp_kallsyms*
	rm -f .tmp_lto.lds
	rm -f .tmp_vmlinux*
	rm -f built-in.a
	rm -f System.map
	rm -f vmlinux
	rm -f vmlinux.o
}

on_exit()
{
	if [ $? -ne 0 ]; then
		cleanup
	fi
}
trap on_exit EXIT

on_signals()
{
	exit 1
}
trap on_signals HUP INT QUIT TERM

#
#
# Use "make V=1" to debug this script
case "${KBUILD_VERBOSE}" in
*1*)
	set -x
	;;
esac

if [ "$1" = "clean" ]; then
	cleanup
	exit 0
fi

# We need access to CONFIG_ symbols
. include/config/auto.conf

# Update version
info GEN .version
if [ -r .version ]; then
	VERSION=$(expr 0$(cat .version) + 1)
	echo $VERSION > .version
else
	rm -f .version
	echo 1 > .version
fi;

# final build of init/
${MAKE} -f "${srctree}/scripts/Makefile.build" obj=init

archive_builtin

#link vmlinux.o
modpost_link vmlinux.o

# modpost vmlinux.o to check for section mismatches
${MAKE} -f "${srctree}/scripts/Makefile.modpost" vmlinux.o

if [ -n "${CONFIG_LTO_CLANG}" ]; then
	# Re-use vmlinux.o, so we can avoid the slow LTO link step in
	# vmlinux_link
	KBUILD_VMLINUX_OBJS=vmlinux.o
	KBUILD_VMLINUX_LIBS=

	# Call recordmcount if needed
	recordmcount vmlinux.o
fi

kallsymso=""
kallsyms_vmlinux=""
if [ -n "${CONFIG_KALLSYMS}" ]; then

	# kallsyms support
	# Generate section listing all symbols and add it into vmlinux
	# It's a three step process:
	# 1)  Link .tmp_vmlinux1 so it has all symbols and sections,
	#     but __kallsyms is empty.
	#     Running kallsyms on that gives us .tmp_kallsyms1.o with
	#     the right size
	# 2)  Link .tmp_vmlinux2 so it now has a __kallsyms section of
	#     the right size, but due to the added section, some
	#     addresses have shifted.
	#     From here, we generate a correct .tmp_kallsyms2.o
	# 2a) We may use an extra pass as this has been necessary to
	#     woraround some alignment related bugs.
	#     KALLSYMS_EXTRA_PASS=1 is used to trigger this.
	# 3)  The correct ${kallsymso} is linked into the final vmlinux.
	#
	# a)  Verify that the System.map from vmlinux matches the map from
	#     ${kallsymso}.

	kallsymso=.tmp_kallsyms2.o
	kallsyms_vmlinux=.tmp_vmlinux2

	# step 1
	vmlinux_link "" .tmp_vmlinux1
	kallsyms .tmp_vmlinux1 .tmp_kallsyms1.o

	# step 2
	vmlinux_link .tmp_kallsyms1.o .tmp_vmlinux2
	kallsyms .tmp_vmlinux2 .tmp_kallsyms2.o

	# step 2a
	if [ -n "${KALLSYMS_EXTRA_PASS}" ]; then
		kallsymso=.tmp_kallsyms3.o
		kallsyms_vmlinux=.tmp_vmlinux3

		vmlinux_link .tmp_kallsyms2.o .tmp_vmlinux3

		kallsyms .tmp_vmlinux3 .tmp_kallsyms3.o
	fi
fi

info LD vmlinux
vmlinux_link "${kallsymso}" vmlinux

if [ -n "${CONFIG_BUILDTIME_EXTABLE_SORT}" ]; then
	info SORTEX vmlinux
	sortextable vmlinux
fi

info SYSMAP System.map
mksysmap vmlinux System.map

# step a (see comment above)
if [ -n "${CONFIG_KALLSYMS}" ]; then
	mksysmap ${kallsyms_vmlinux} .tmp_System.map

	if ! cmp -s System.map .tmp_System.map; then
		echo >&2 Inconsistent kallsyms data
		echo >&2 Try "make KALLSYMS_EXTRA_PASS=1" as a workaround
		exit 1
	fi
fi
