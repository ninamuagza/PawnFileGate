#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Available configs: Debug, [RelWithDebInfo], Release
[[ -z "$CONFIG" ]] \
&& config=RelWithDebInfo \
|| config="$CONFIG"
# Available options: [true], false
[[ -z "$BUILD_SERVER" ]] \
&& build_server=1 \
|| build_server="$BUILD_SERVER"
[[ -z "$BUILD_DIR" ]] \
&& build_dir=build \
|| build_dir="$BUILD_DIR"
[[ -z "$PAWNREST_ENABLE_TLS" ]] \
&& tls_enabled=OFF \
|| tls_enabled="$PAWNREST_ENABLE_TLS"
[[ -z "$PAWNREST_TLS_STATIC_OPENSSL" ]] \
&& tls_static=OFF \
|| tls_static="$PAWNREST_TLS_STATIC_OPENSSL"
[[ -z "$DOCKER_IMAGE" ]] \
&& docker_image="" \
|| docker_image="$DOCKER_IMAGE"
[[ -z "$DOCKERFILE_DIR" ]] \
&& docker_dir="" \
|| docker_dir="$DOCKERFILE_DIR"

if [[ -z "$docker_image" || -z "$docker_dir" ]]; then
    if [[ "${tls_enabled^^}" == "ON" && "${tls_static^^}" == "ON" ]]; then
        docker_image="omp-easing-functions/build:ubuntu-22.04-static-ssl"
        docker_dir="${SCRIPT_DIR}/build_ubuntu-22.04-static-ssl/"
    else
        docker_image="omp-easing-functions/build:ubuntu-18.04"
        docker_dir="${SCRIPT_DIR}/build_ubuntu-18.04/"
    fi
fi

if ! docker image inspect "${docker_image}" >/dev/null 2>&1; then
    docker pull "${docker_image}" >/dev/null 2>&1 || \
    docker build -t "${docker_image}" "${docker_dir}" || exit 1
fi

folders=("${build_dir}")
for folder in "${folders[@]}"; do
    if [[ ! -d "${SCRIPT_DIR}/${folder}" ]]; then
        mkdir "${SCRIPT_DIR}/${folder}" &&
        chown 1000:1000 "${SCRIPT_DIR}/${folder}" || exit 1
    fi
done

docker run \
    --rm \
    -t \
    -w /code \
    -v "${REPO_ROOT}:/code" \
    -v "${SCRIPT_DIR}/${build_dir}:/code/${build_dir}" \
    -e CONFIG=${config} \
    -e BUILD_SERVER=${build_server} \
    -e BUILD_DIR=${build_dir} \
    -e PAWNREST_ENABLE_TLS=${tls_enabled} \
    -e PAWNREST_TLS_STATIC_OPENSSL=${tls_static} \
    "${docker_image}"
