#!/usr/bin/env bash

python=${root:?}/python
python_source=${python:?}
python_git=https://github.com/python/cpython.git
python_ref=
python_stage=${python:?}/stage

go--python-clone() {
    source=${python_source:?} \
    git=${python_git:?} \
    ref=${python_ref:-} \
    go--git-clone
}

go--python-configure() (
    cd "${python_source:?}" && \
    ./configure \
        --prefix "${python_stage:?}" \
        --enable-ipv6 \
        --enable-shared \
        --with-computed-gotos=yes \
        --with-dbmliborder=gdbm:ndbm:bdb \
        --with-system-expat \
        --with-system-ffi \
        --enable-loadable-sqlite-extensions \
        --with-dtrace \
        --with-ssl-default-suites=openssl \
        --with-ensurepip \
        --disable-optimizations
)

go--python-build() (
    cd "${python:?}" && \
    make
)

go--python-install() (
    cd "${python:?}" && \
    make \
        install
)

go--python-exec() {
    LD_LIBRARY_PATH=${python_stage:?}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH:?}} \
    PATH=${python_stage:?}/bin${PATH:+:${PATH:?}} \
    "$@"
}