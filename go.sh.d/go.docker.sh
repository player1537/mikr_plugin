#!/usr/bin/env bash

[ -n "${__GO_DOCKER_SH__+isset}" ] || {
__GO_DOCKER_SH__=

tag=${project:?}_${USER:?}:latest
name=${project:?}_${USER:?}
dockerfile=
publish=
bind=
port=
user=$(id -u):$(id -g)
env=()
workdir=
network=

go--docker-pull() (
    docker pull \
        ${tag:?}
)

go--docker-build() (
    docker build \
        --tag ${tag:?} \
        ${dockerfile:+--file ${dockerfile:?}} \
        "$@" \
        ${root:?}
)

go--docker-run() (
    docker run \
        --detach \
        --mount type=bind,src="${HOME:?}",dst="${HOME:?}" \
        --name "${name:?}" \
        ${port:+--publish ${bind:+${bind:?}:}${port:?}:${port:?}} \
        ${publish:+--publish ${publish:?}} \
        ${user:+--user ${user:?}} \
        ${env:+--env ${env:?}} \
        ${network:+--network ${network:?}} \
        "${tag:?}" \
        "$@"
)

go--docker-stop() (
    docker stop \
        "$@" \
        "${name:?}"
)

go--docker-rm() (
    docker rm \
        "$@" \
        "${name:?}"
)

go--docker-exec() (
    docker exec \
        --detach-keys "ctrl-q,ctrl-q" \
        ${user:+--user ${user:?}} \
        --interactive \
        ${workdir:+--workdir "${workdir:?}"} \
        ${env:+--env ${env:?}} \
        "${name:?}" \
        "$@"
)

}  # __GO_DOCKER_SH__