#! /bin/sh

# Run all exe's in the tst directory under valgrind.
for i in `find tst -type f -perm +111`; do
	echo $i
	valgrind -q --leak-check=yes $i 2>&1 | \
		egrep '= (definitely lost|possibly lost|still reachable)'
done

exit 0;
