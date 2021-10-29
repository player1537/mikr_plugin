#!/usr/bin/env bash

go--x11docker-exec() ( # subshell
    while ! mkdir "${root:?}"/.x11docker; do
        sleep 1
    done
    trap 'rm -r "${root:?}"/.x11docker' EXIT
    {
        printf $'#!/usr/bin/env bash\n'
        printf $'exec'
        printf $' %q' "$@"
        printf $'\n'
    } > "${root:?}"/.x11docker/run.sh
    chmod +x "${root:?}"/.x11docker/run.sh
    x11docker --net=host --gpu --hostdisplay --home="${HOME:?}" --workdir="${PWD:?}" "${tag:?}" -- "${root:?}"/.x11docker/run.sh
)
