#! /bin/sh -e

if [ ! -r VERSION -o ! -r vstr.spec -o ! -r configure ]; then
 echo "No VERSION, vstr.spec or configure file."
 exit 1
fi

v="`cat VERSION`"
s="`pwd`"
cd ../build/vstr

rm -rf vstr-$v
cp -a $s ./vstr-$v
cd ./vstr-$v
./scripts/clean.sh

find . \
 \( -name "*.o" -o -name ".*[%~]" -o -name "*[%~]" -o -name "#*#" \) \
 -print0 | xargs --no-run-if-empty -0 rm -f

cp $s/vstr.spec .

cd ..

tar -cf vstr-$v.tar vstr-$v
bzip2 -9f vstr-$v.tar

tar -cf vstr-$v.tar vstr-$v
gzip -9f vstr-$v.tar

sudo rpmbuild -ta                  vstr-$v.tar.gz
sudo rpmbuild -tb --define 'dbg 1' vstr-$v.tar.gz

echo "/usr/src/redhat/RPMS/*/vstr*-$v-*"
echo "/usr/src/redhat/SRPMS/vstr*-$v-*"

ls -aslF /usr/src/redhat/RPMS/*/vstr*-$v-*
ls -aslF /usr/src/redhat/SRPMS/vstr*-$v-*

