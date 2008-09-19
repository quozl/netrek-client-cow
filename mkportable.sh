#!/bin/sh

# mkportable.sh
#
# produces a tar.gz consisting of a binary and dependent libraries.

set -e

BINARY=netrek
ALIAS=netrek-client-cow

if [ ! -x ${BINARY} ]; then
    echo "mkportable: error, you must make netrek first"
    exit 1
fi

if [ ! -x name ]; then
    echo "mkportable: error, you must make name first"
    exit 1
fi

PACKAGE=netrek-client-cow
VERSION=`./name`
ARCH=i386
RELEASE=0
DESTDIR=${PACKAGE}-${VERSION}-${ARCH}-portable-${RELEASE}

if [ -d ${DESTDIR} ]; then
    echo "mkportable: error, old destination directory ${DESTDIR} exists"
    exit 1
fi

LIBDIR=/lib
BINDIR=/

mkdir --parents ${DESTDIR}${LIBDIR}
mkdir --parents ${DESTDIR}${BINDIR}

# place the binary
cp --preserve=mode,ownership,timestamps ${BINARY} ${DESTDIR}${LIBDIR}/

# place the dependencies
for library in `ldd ${BINARY}|egrep -v "linux-gate.so|ld-linux.so"|awk '{print $3}'`; do
	cp --preserve=mode,ownership,timestamps ${library} ${DESTDIR}${LIBDIR}/
done

# prepare wrapper
cat > ${DESTDIR}${BINDIR}/${ALIAS} <<EOF
#!/bin/sh

# look only in the package directory for libraries
export LD_LIBRARY_PATH=.${LIBDIR}

# set path to where binary is expected
export PATH=.${LIBDIR}:\${PATH}

# run the program giving the arguments you gave to us
exec ${BINARY} \$*
EOF

chmod +x ${DESTDIR}${BINDIR}/${ALIAS}

# tar it up
tar --create --gzip --file ${DESTDIR}.tar.gz ${DESTDIR}

# and nuke
rm -rf ${DESTDIR}

# say hello
echo ${DESTDIR}.tar.gz
