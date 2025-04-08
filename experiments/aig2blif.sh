#!/bin/bash
#echo "Script PATH: $PATH"
export PATH=$PATH:/home/xiaokewan/Software/vtr-verilog-to-routing-master/abc

BENCH_DIR="./benchmarks"
OUTPUT_DIR="$BENCH_DIR/blif_folder"
mkdir -p "$OUTPUT_DIR"

if ! command -v abc &> /dev/null; then
    echo "Error: abc command not found. Make sure ABC is in your PATH or aliased."
    exit 1
fi

for aig_file in "$BENCH_DIR"/*.aig; do
    base_name=$(basename "$aig_file" .aig)
    blif_file="$OUTPUT_DIR/$base_name.blif"
    
    echo "Converting $aig_file -> $blif_file"
    abc -c "read $aig_file; strash; if -K 6; write_blif $blif_file"

done

echo "All AIG files have been converted to BLIF in $OUTPUT_DIR."
