: '@(#)years.sh	2.1'

: years lo hi type

case $# in
	3)		;;
	*)		echo "$0: usage is $0 lo hi type" 1>&2
			exit 1 ;;
esac

lo="$1"
hi="$2"
type="$3"

case $type in
	uspres)		check='(y % 4) == 0' ;;
	nonpres)	check='(y % 4) != 0' ;;	
	*)		echo "$0: wild year type ($type)" 1>&2
			exit 1 ;;
esac

exec awk "
BEGIN {
	for (y = $lo; y <= $hi; ++y)
		if ($check)
			print y;
	exit
}
"
