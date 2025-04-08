#!/bin/bash

export PATH=$PATH:/home/xiaokewan/Software/vtr-verilog-to-routing-master/vpr

# Default architecture
ARCH_NAME="stratix10"

# Argument parsing
BENCHMARKS=()
USER_SPECIFIED=false
while [[ "$#" -gt 0 ]]; do
  case $1 in
    -b|--benchmarks)
      shift
      USER_SPECIFIED=true
      while [[ "$1" != "" && "${1:0:1}" != "-" ]]; do
        BENCHMARKS+=("$1")
        shift
      done
      ;;
    -a|--arch)
      shift
      ARCH_NAME="$1"
      shift
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

# Resolve architecture path
case "$ARCH_NAME" in
  stratix10)
    ARCH_FILE="$(realpath "$VTR_ROOT/vtr_flow/arch/titan/stratix10_arch.timing.xml")"
    ;;
  stratixiv)
    ARCH_FILE="$(realpath "$VTR_ROOT/vtr_flow/arch/titan/stratixiv_arch.timing.xml")"
    ;;
  *)
    echo "Unsupported architecture: $ARCH_NAME"
    exit 1
    ;;
esac

BLIF_DIR="$(realpath "./benchmarks/blif_folder")"
OUTPUT_ROOT="$(realpath "./vpr_runs_${ARCH_NAME}")"

MAX_SIZE=$((1 * 1024 * 1024))
MAX_PARALLEL_JOBS=6

mkdir -p "$OUTPUT_ROOT"

run_vpr_job() {
  blif_path="$1"
  ARCH_FILE="$2"
  OUTPUT_ROOT="$3"
  force="$4"  # "yes" if size check should be skipped

  file_size=$(stat -c%s "$blif_path")
  if [ "$force" != "yes" ] && [ "$file_size" -ge "$MAX_SIZE" ]; then
    echo "Skipping $(basename "$blif_path") (size: $((file_size / 1024)) KB > 1 MB)"
    return
  fi

  blif_file=$(basename "$blif_path")
  blif_name="${blif_file%.blif}"
  run_dir="$OUTPUT_ROOT/$blif_name"
  mkdir -p "$run_dir"

  echo "Running VPR for $blif_file in $run_dir with architecture $ARCH_NAME"

  (
    cd "$run_dir"
    vpr "$ARCH_FILE" "$blif_path" --route_chan_width 250 
  )
}

export -f run_vpr_job
export ARCH_FILE MAX_SIZE ARCH_NAME

BLIF_PATHS=()

if [ "$USER_SPECIFIED" = true ]; then
  for name in "${BENCHMARKS[@]}"; do
    orig="$BLIF_DIR/$name.blif"
    dup="$BLIF_DIR/${name}_dup.blif"

    [ -f "$orig" ] && BLIF_PATHS+=("$orig:normal")
    [ -f "$dup" ] && BLIF_PATHS+=("$dup:force")
  done
else
  for filepath in "$BLIF_DIR"/*.blif; do
    base=$(basename "$filepath" .blif)

    if [[ "$base" == *_dup ]]; then
      continue
    fi

    orig="$BLIF_DIR/$base.blif"
    dup="$BLIF_DIR/${base}_dup.blif"

    file_size=$(stat -c%s "$orig")
    if [ "$file_size" -lt "$MAX_SIZE" ]; then
      [ -f "$orig" ] && BLIF_PATHS+=("$orig:normal")
      [ -f "$dup" ] && BLIF_PATHS+=("$dup:force")
    fi
  done
fi

# Run in parallel
printf "%s\n" "${BLIF_PATHS[@]}" | parallel -j 6 --colsep ':' run_vpr_job {1} "$ARCH_FILE" "$OUTPUT_ROOT" {2}

