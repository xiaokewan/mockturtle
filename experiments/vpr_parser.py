import os
import re
import csv
import sys


def parse_log_file(filepath):
    with open(filepath, 'r') as file:
        content = file.read()

    time_regex = r"The entire flow of VPR took ([\d.]+) seconds"
    cpd_regex = r"Final critical path delay \(least slack\): ([\d.]+) ns"
    cp_regex = r"Critical path:\s+([\d.]+) ns"
    wirelength_regex = r"Total wirelength: ([\d]+)"
    packing_time_regex = r"# Packing took ([\d.]+) seconds"
    routing_time_regex = r"# Routing took ([\d.]+) seconds"
    placement_time_regex = r"Placement Total  timing analysis took ([\d.]+) seconds"
    fpga_size_regex = r"FPGA sized to (\d+) x (\d+) \(auto\)"
    block_count_regex = r"Circuit Statistics:\s+Blocks:\s+(\d+)"
    net_count_regex = r"Nets\s+:\s+(\d+)"
    fanout_avg_regex = r"Avg Fanout:\s+([\d.]+)"
    fanout_max_regex = r"Max Fanout:\s+([\d.]+)"
    fanout_min_regex = r"Min Fanout:\s+([\d.]+)"
    device_util_regex = r"Device Utilization:\s+([\d.]+) \(target"

    time = re.search(time_regex, content)
    cpd = re.search(cpd_regex, content)
    if cpd is None:
    	cpd = re.search(cp_regex, content)
    wirelength = re.search(wirelength_regex, content)
    packing_time = re.search(packing_time_regex, content)
    routing_time = re.search(routing_time_regex, content)
    placement_time = re.search(placement_time_regex, content)
    fpga_size = re.search(fpga_size_regex, content)
    block_count = re.search(block_count_regex, content)
    net_count = re.search(net_count_regex, content)
    fanout_avg = re.search(fanout_avg_regex, content)
    fanout_max = re.search(fanout_max_regex, content)
    fanout_min = re.search(fanout_min_regex, content)
    device_util = re.search(device_util_regex, content)

    if fpga_size:
        width = int(fpga_size.group(1))
        height = int(fpga_size.group(2))
        area = width * height
        size_str = f"{width} x {height}"
    else:
        area = size_str = None

    return {
        "time": float(time.group(1)) if time else None,
        "cpd": float(cpd.group(1)) if cpd else None,
        "total_wirelength": int(wirelength.group(1)) if wirelength else None,
        "packing_time": float(packing_time.group(1)) if packing_time else None,
        "routing_time": float(routing_time.group(1)) if routing_time else None,
        "placement_time": float(placement_time.group(1)) if placement_time else None,
        "fpga_size": size_str,
        "fpga_area": area,
        "num_blocks": int(block_count.group(1)) if block_count else None,
        "num_nets": int(net_count.group(1)) if net_count else None,
        "fanout_avg": float(fanout_avg.group(1)) if fanout_avg else None,
        "fanout_max": float(fanout_max.group(1)) if fanout_max else None,
        "fanout_min": float(fanout_min.group(1)) if fanout_min else None,
        "device_utilization": float(device_util.group(1)) if device_util else None
    }


def save_to_csv(data, filename):
    # Ensure benchmark is first column
    if data:
        fieldnames = ["benchmark"] + [k for k in data[0] if k != "benchmark"]
    else:
        print("No data found.")
        return

    with open(filename, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        for row in data:
            writer.writerow(row)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python readvpr.py <log_folder> <output_folder>")
        sys.exit(1)

    log_folder = sys.argv[1]
    output_folder = sys.argv[2]

    log_files = [
        os.path.join(root, f)
        for root, dirs, files in os.walk(log_folder)
        for f in files if f == "vpr_stdout.log"
    ]

    data = []
    for filepath in log_files:
        benchmark_name = os.path.basename(os.path.dirname(filepath))
        log_data = {"benchmark": benchmark_name}
        log_data.update(parse_log_file(filepath))
        data.append(log_data)

    # Sort by benchmark name
    data = sorted(data, key=lambda x: x['benchmark'])

    output_csv = os.path.join(output_folder, os.path.basename(os.path.dirname(log_folder)) + '_vpr_data.csv')
    save_to_csv(data, output_csv)

    print(f"Saved VPR data to {output_csv}")
