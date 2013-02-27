autoheader
aclocal
autoconf
libtoolize -f -c
automake -a
rm aclocal.m4
#./configure  --enable-unit-test --enable-multi-evt-driver
#make