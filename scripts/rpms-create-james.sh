#! /bin/bash -e

if [ ! -r VERSION -o ! -r vstr.spec -o ! -r configure ]; then
  if [ -r configure ]; then
    ./scripts/b/DOCS.sh
  else
    echo "No VERSION, vstr.spec or configure file." &>2
    exit 1
  fi
fi

v="`cat VERSION`"
s="`pwd`"
cd ../build/vstr

rm -rf vstr-$v
cp -a $s ./vstr-$v
cd ./vstr-$v

./scripts/clean.sh full

# Perf output...
rm -rf ./examples/perf*

# Backup files...
find . \
 \( -name "*.o" -o -name ".*[%~]" -o -name "*[%~]" -o -name "#*#" \) \
 -print0 | xargs --no-run-if-empty -0 rm -f

# Arch stuff...
rm -rf ./{arch}
find . -name .arch-ids -type d -print0 | xargs -0 rm -rf

# Create tarballs/RPMS
cp $s/vstr.spec .

cd ..

tar -cf vstr-$v.tar vstr-$v
bzip2 -9f vstr-$v.tar

tar -cf vstr-$v.tar vstr-$v
gzip -9f vstr-$v.tar

sudo rpmbuild -ta --define 'chk 1' vstr-$v.tar.gz

echo "/usr/src/redhat/RPMS/*/vstr*-$v-*"
echo "/usr/src/redhat/SRPMS/vstr*-$v-*"

ls -aslF /usr/src/redhat/RPMS/*/vstr*-$v-*
ls -aslF /usr/src/redhat/SRPMS/vstr*-$v-*

