# from http://mywiki.wooledge.org/BashFAQ/021

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
