#!/usr/bin/env bash
# scripts/docker-run.sh
#
# Build KoraOS inside the axfab/kora-gcc Docker image (no local cross-
# toolchain needed), assemble a bootable GRUB ISO, and optionally boot it
# under QEMU on the host.
#
# Usage:
#   scripts/docker-run.sh [OPTIONS] [COMMAND...]
#
# Commands (default: all):
#   build   Compile the kernel and drivers inside Docker
#   iso     Assemble the bootable ISO inside Docker  (implies build)
#   run     Launch QEMU on the host with the built ISO
#   all     build + iso + run
#
# Options:
#   -m ARCH   Target architecture.  Supported: i386 (default)
#   -j N      Parallel make jobs (default: number of CPU cores)
#   -o DIR    Output directory for the ISO (default: <repo-root>/build)
#   -n        Dry-run: print Docker / QEMU commands without executing them
#   -h        Show this help
#
# Prerequisites:
#   build/iso : Docker must be running
#   run       : qemu-system-i386 (brew install qemu  /  apt install qemu-system-x86)
# ------------------------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# ── Defaults ──────────────────────────────────────────────────────────────────
DOCKER_IMAGE="axfab/kora-gcc:latest"
ARCH="i386"
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
OUTPUT_DIR="$REPO_ROOT/build"
DRY_RUN=false
COMMANDS=()

# ── Colours ───────────────────────────────────────────────────────────────────
if [ -t 1 ]; then
    C_BOLD='\033[1m'; C_GREEN='\033[0;32m'; C_CYAN='\033[0;36m'
    C_YELLOW='\033[0;33m'; C_RED='\033[0;31m'; C_RESET='\033[0m'
else
    C_BOLD=''; C_GREEN=''; C_CYAN=''; C_YELLOW=''; C_RED=''; C_RESET=''
fi

info()  { echo -e "${C_CYAN}==>${C_RESET}${C_BOLD} $*${C_RESET}"; }
ok()    { echo -e "${C_GREEN} ✓${C_RESET} $*"; }
warn()  { echo -e "${C_YELLOW}[warn]${C_RESET} $*"; }
die()   { echo -e "${C_RED}[error]${C_RESET} $*" >&2; exit 1; }

run() {
    if $DRY_RUN; then
        echo -e "${C_YELLOW}[dry-run]${C_RESET} $*"
    else
        "$@"
    fi
}

# ── Argument parsing ──────────────────────────────────────────────────────────
usage() {
    sed -n '/^# Usage:/,/^# -/p' "$0" | sed 's/^# \?//'
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -m) ARCH="$2";       shift 2 ;;
        -j) JOBS="$2";       shift 2 ;;
        -o) OUTPUT_DIR="$2"; shift 2 ;;
        -n) DRY_RUN=true;    shift   ;;
        -h|--help) usage ;;
        build|iso|run|all) COMMANDS+=("$1"); shift ;;
        *) die "Unknown argument: $1.  Try -h for help." ;;
    esac
done

[[ ${#COMMANDS[@]} -eq 0 ]] && COMMANDS=("all")

# ── Architecture configuration ────────────────────────────────────────────────
case "$ARCH" in
    i386)
        CROSS="/i386-kora/bin/i386-kora-"
        TARGET="i386-pc-kora"
        KNAME="kora-i386.krn"
        QEMU_BIN="qemu-system-i386"
        QEMU_FLAGS="--serial stdio --smp 2 -m 64"
        # QEMU_FLAGS="--cdrom KoraOs.iso --serial stdio --smp 2 -m 64"
        ;;
    *)
        die "Unsupported architecture '$ARCH'.  Currently only i386 is implemented."
        ;;
esac

ISO_FILE="$OUTPUT_DIR/KoraOs.iso"

# ── Helper: check a command exists on the host ────────────────────────────────
need_host() {
    if ! command -v "$1" &>/dev/null; then
        echo ""
        die "'$1' not found on the host.\n  macOS : brew install $2\n  Debian: apt install $3"
    fi
}

# ── STEP 1: build (kernel + drivers, inside Docker) ──────────────────────────
cmd_build() {
    info "Building KoraOS ($ARCH) inside Docker…"
    need_host docker docker docker.io

    # The build script runs entirely inside the container.
    # Source is mounted read-write so obj/, bin/, lib/ land in the repo tree
    # (same behaviour as a local make).
    run docker run --rm \
        -v "$REPO_ROOT:/src" \
        -w /src \
        "$DOCKER_IMAGE" \
        bash -c "
            set -e
            echo '==> Kernel'
            make -j${JOBS} target=${TARGET} CROSS='${CROSS}' NOCOV=y NODEPS=y

            echo '==> Drivers'
            make -j${JOBS} target=${TARGET} CROSS='${CROSS}' NOCOV=y NODEPS=y drivers
        "
    ok "Build complete.  Kernel: bin/$KNAME"
}

# ── STEP 2: iso (assemble bootable ISO, inside Docker) ───────────────────────
cmd_iso() {
    cmd_build

    info "Assembling bootable ISO inside Docker…"

    # Resolve the driver tar relative to the output dir so it survives outside
    # the container.
    run mkdir -p "$OUTPUT_DIR"

    run docker run --rm \
        -v "$REPO_ROOT:/src" \
        -v "$OUTPUT_DIR:/out" \
        -w /src \
        "$DOCKER_IMAGE" \
        bash -c "
            set -e

            # ── ISO directory tree ──────────────────────────────────────────
            rm -rf /tmp/iso
            mkdir -p /tmp/iso/boot/grub
            mkdir -p /tmp/iso/boot/mods

            # Kernel image
            cp bin/${KNAME} /tmp/iso/boot/${KNAME}

            # GRUB config (patch multiboot path to match the actual binary name)
            sed 's|multiboot /boot/kora-x86.krn|multiboot /boot/${KNAME}|g' \
                scripts/cfg/grub.cfg > /tmp/iso/boot/grub/grub.cfg

            # ── Driver ramdisk ──────────────────────────────────────────────
            # Build each enabled driver and pack them into a tar archive that
            # the kernel's kloader thread scans at boot.
            DRV_TMP=/tmp/drv_mods
            rm -rf \$DRV_TMP && mkdir -p \$DRV_TMP/bin

            parse_yaml() {
                local s='[[:space:]]*' w='[a-zA-Z0-9_]*' fs=\$(echo @|tr @ '\034')
                sed -ne \"s|^\(\$s\):\|\1|\" \
                    -e \"s|^\(\$s\)\(\$w\)\$s:\$s[\\\"\\']\(.*\)[\\\"\\']\$s\\\$|\1\$fs\2\$fs\3|p\" \
                    -e \"s|^\(\$s\)\(\$w\)\$s:\$s\(.*\)\$s\\\$|\1\$fs\2\$fs\3|p\" \"\$1\" |
                awk -F\$fs '{
                    indent=length(\$1)/2; vname[indent]=\$2;
                    for(i in vname){ if(i>indent){delete vname[i]} }
                    if(length(\$3)>0){
                        vn=\"\";
                        for(i=0;i<indent;i++){vn=(vn)(vname[i])(\"_\")}
                        printf(\"%s%s%s=\\\"%s\\\"\\n\",\"\$2\",vn,\$2,\$3)
                    }
                }'
            }

            if [ -f config.yml ]; then
                eval \$(parse_yaml config.yml drivers_)
                for dir in \$(find lib/ -name '*.ko' 2>/dev/null | xargs dirname | sort -u); do
                    cp \$dir/*.ko \$DRV_TMP/bin/ 2>/dev/null || true
                done
                # Also pick up any .ko files directly in lib/
                cp lib/*.ko \$DRV_TMP/bin/ 2>/dev/null || true
            fi

            cd \$DRV_TMP
            tar cf /tmp/iso/boot/x86.miniboot.tar bin
            cd /src

            # ── Create ISO ──────────────────────────────────────────────────
            grub-mkrescue -o /out/KoraOs.iso /tmp/iso
            ls -lh /out/KoraOs.iso
        "
    ok "ISO ready: $ISO_FILE"
}

# ── STEP 3: run (QEMU on the host) ───────────────────────────────────────────
cmd_run() {
    need_host "$QEMU_BIN" qemu "qemu-system-x86"

    [[ -f "$ISO_FILE" ]] || die "ISO not found: $ISO_FILE\n  Run 'scripts/docker-run.sh iso' first."

    info "Booting $ISO_FILE with $QEMU_BIN…"
    info "  Serial output → stdout.  Press Ctrl-A X to quit QEMU."
    echo ""

    # KVM speeds up i386 emulation on Linux x86 hosts; silently skip on macOS.
    EXTRA_FLAGS=""
    if [[ "$(uname -s)" == "Linux" ]] && [[ "$ARCH" == "$(uname -m)" ]]; then
        EXTRA_FLAGS="--enable-kvm"
    fi

    run "$QEMU_BIN" \
        --cdrom "$ISO_FILE" \
        --serial stdio \
        --smp 2 \
        -m 32 \
        -display none \
        $EXTRA_FLAGS
}

simple_run() {

    [[ -f "bin/kora-i386.krn" ]] || die "Kernel not found: kora-i386.krn\n  Run 'scripts/docker-run.sh build' first."
    [[ -d "lib" ]] || die "No drivers found at lib/*\n  Run 'scripts/docker-run.sh build' first."

    info "Booting kora-i386.krn with ${QEMU_BIN}…"
    info "  Serial output → stdout.  Press Ctrl-A X to quit QEMU."
    echo ""

    # KVM speeds up i386 emulation on Linux x86 hosts; silently skip on macOS.
    EXTRA_FLAGS=""
    if [[ "$(uname -s)" == "Linux" ]] && [[ "$ARCH" == "$(uname -m)" ]]; then
        EXTRA_FLAGS="--enable-kvm"
    fi

    cd lib
    # COPYFILE_DISABLE=1 suppresses macOS AppleDouble "._filename" resource-fork
    # entries that macOS tar otherwise injects silently into the archive.
    COPYFILE_DISABLE=1 tar cf ../initrd.tar *
    cd ..
    run "$QEMU_BIN" \
        --kernel bin/kora-i386.krn --initrd initrd.tar \
        --serial stdio \
        --smp 2 \
        -m 32 \
        -display none \
        $EXTRA_FLAGS
}

# ── Dispatch ──────────────────────────────────────────────────────────────────
echo -e "${C_BOLD}KoraOS docker-run — arch=${ARCH}  jobs=${JOBS}${C_RESET}"
echo ""

for CMD in "${COMMANDS[@]}"; do
    case "$CMD" in
        build) cmd_build ;;
        # iso)   cmd_iso   ;;
        run)   simple_run   ;;
        all)   cmd_build && simple_run ;;
    esac
done
