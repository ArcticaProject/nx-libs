# from http://mywiki.wooledge.org/BashFAQ/021

# The ${a/b/c} substitution is not POSIX compatible. Additionally, in
# bash 3.x, quotes do not escape slashes. This causes screwed up
# installation paths.
#
#     SLES 11, bash-3.2-147.9.13
#     $ dirname="foo/bar"
#     $ echo ${dirname//"foo/bar"/"omg/nei"}
#     bar/omg/nei/bar
#
#     openSUSE 12.2, bash-4.2-51.6.1
#     $ dirname="foo/bar"
#     $ echo ${dirname//"foo/bar"/"omg/nei"}
#     omg/nei
#
#     openSUSE 12.2, dash-0.5.7-5.1.2.x86_64
#     $ dirname="foo/bar"
#     $ echo ${dirname//"foo/bar"/"omg/nei"}
#     dash: 2: Bad substitution
#
# Source this file into your bash scripts to make available
# a replacement (the string_rep function) for this substitution
# mess.
#

string_rep()
{
	# initialize vars
	in=$1
	unset out

	# SEARCH must not be empty
	test -n "$2" || return

	while true; do
		# break loop if SEARCH is no longer in "$in"
		case "$in" in
			*"$2"*) : ;;
			*) break;;
		esac

		# append everything in "$in", up to the first instance of SEARCH, and REP, to "$out"
		out=$out${in%%"$2"*}$3
		# remove everything up to and including the first instance of SEARCH from "$in"
		in=${in#*"$2"}
	done

	# append whatever is left in "$in" after the last instance of SEARCH to out, and print
	printf '%s%s\n' "$out" "$in"
}
