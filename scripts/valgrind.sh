#! /bin/sh

# unhappy on 8.0 (glibc-2.3.x) atm.
exit 0;

# Run all exe's in the tst directory under valgrind.
for i in tst/*/tst_*; do
   if [ -x $i ]; then # Skip .o
	echo $i
	valgrind -q --leak-check=yes $i 2>&1 | \
		egrep '= (definitely lost|possibly lost|still reachable)'
   fi
done

exit 0;
