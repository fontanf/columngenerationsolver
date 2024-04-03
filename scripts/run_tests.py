import argparse
import sys
import os

parser = argparse.ArgumentParser(description='')
parser.add_argument('directory')
parser.add_argument(
        "-t", "--tests",
        type=str,
        nargs='*',
        help='')

args = parser.parse_args()


if args.tests is None or "bin-packing-with-conflicts" in args.tests:
    print("Bin packing problem with conflicts")
    print("----------------------------------")
    print()

    bin_packing_with_conflicts_data = [
            (os.path.join("muritiba2010", "BPPC_1_0_1.txt"), "default"),
            (os.path.join("muritiba2010", "BPPC_1_1_2.txt"), "default"),
            (os.path.join("muritiba2010", "BPPC_1_2_3.txt"), "default"),
            (os.path.join("muritiba2010", "BPPC_1_3_4.txt"), "default"),
            (os.path.join("muritiba2010", "BPPC_1_4_5.txt"), "default"),
            (os.path.join("muritiba2010", "BPPC_1_5_6.txt"), "default"),
            (os.path.join("muritiba2010", "BPPC_1_6_7.txt"), "default"),
            (os.path.join("muritiba2010", "BPPC_1_7_8.txt"), "default"),
            (os.path.join("muritiba2010", "BPPC_1_8_9.txt"), "default"),
            (os.path.join("muritiba2010", "BPPC_1_9_10.txt"), "default")]
    bin_packing_with_conflicts_main = os.path.join(
            "bazel-bin",
            "examples",
            "bin_packing_with_conflicts_main")
    for instance, instance_format in bin_packing_with_conflicts_data:
        instance_path = os.path.join(
                "data",
                "bin_packing_with_conflicts",
                instance)
        json_output_path = os.path.join(
                args.directory,
                "bin_packing_with_conflicts",
                instance + ".json")
        if not os.path.exists(os.path.dirname(json_output_path)):
            os.makedirs(os.path.dirname(json_output_path))
        command = (
                bin_packing_with_conflicts_main
                + "  --verbosity-level 1"
                + "  --input \"" + instance_path + "\""
                + " --format \"" + instance_format + "\""
                + "  --algorithm greedy"
                + " --internal-diving 1"
                + "  --output \"" + json_output_path + "\"")
        print(command)
        os.system(command)
        print()
    print()
    print()


if args.tests is None or "capacitated-vehicle-routing" in args.tests:
    print("Capacitated vehicle routing problem")
    print("-----------------------------------")
    print()

    capacitated_vehicle_routing_data = [
            (os.path.join("uchoa2014", "X", "X-n101-k25.vrp"), "cvrplib"),
            (os.path.join("uchoa2014", "X", "X-n106-k14.vrp"), "cvrplib"),
            (os.path.join("uchoa2014", "X", "X-n110-k13.vrp"), "cvrplib"),
            (os.path.join("uchoa2014", "X", "X-n115-k10.vrp"), "cvrplib"),
            (os.path.join("uchoa2014", "X", "X-n120-k6.vrp"), "cvrplib"),
            (os.path.join("uchoa2014", "X", "X-n125-k30.vrp"), "cvrplib"),
            (os.path.join("uchoa2014", "X", "X-n129-k18.vrp"), "cvrplib"),
            (os.path.join("uchoa2014", "X", "X-n134-k13.vrp"), "cvrplib"),
            (os.path.join("uchoa2014", "X", "X-n139-k10.vrp"), "cvrplib"),
            (os.path.join("uchoa2014", "X", "X-n143-k7.vrp"), "cvrplib")]
    capacitated_vehicle_routing_main = os.path.join(
            "bazel-bin",
            "examples",
            "capacitated_vehicle_routing_main")
    for instance, instance_format in capacitated_vehicle_routing_data:
        instance_path = os.path.join(
                "data",
                "capacitated_vehicle_routing",
                instance)
        json_output_path = os.path.join(
                args.directory,
                "capacitated_vehicle_routing",
                instance + ".json")
        if not os.path.exists(os.path.dirname(json_output_path)):
            os.makedirs(os.path.dirname(json_output_path))
        command = (
                capacitated_vehicle_routing_main
                + "  --verbosity-level 1"
                + "  --input \"" + instance_path + "\""
                + " --format \"" + instance_format + "\""
                + "  --algorithm limited-discrepancy-search"
                + " --automatic-stop 1"
                + "  --output \"" + json_output_path + "\"")
        print(command)
        os.system(command)
        print()
    print()
    print()


if args.tests is None or "cutting-stock" in args.tests:
    print("Cutting stock problem")
    print("---------------------")
    print()

    cutting_stock_data = [
            (os.path.join("delorme2016", "RG_CSP", "BPP_50_50_0.1_0.7_0.txt"), "bpplib_csp"),
            (os.path.join("delorme2016", "RG_CSP", "BPP_50_50_0.1_0.8_1.txt"), "bpplib_csp"),
            (os.path.join("delorme2016", "RG_CSP", "BPP_50_50_0.2_0.7_2.txt"), "bpplib_csp"),
            (os.path.join("delorme2016", "RG_CSP", "BPP_50_50_0.2_0.8_3.txt"), "bpplib_csp"),
            (os.path.join("delorme2016", "RG_CSP", "BPP_50_75_0.1_0.7_4.txt"), "bpplib_csp"),
            (os.path.join("delorme2016", "RG_CSP", "BPP_50_75_0.1_0.8_5.txt"), "bpplib_csp"),
            (os.path.join("delorme2016", "RG_CSP", "BPP_50_75_0.2_0.7_6.txt"), "bpplib_csp"),
            (os.path.join("delorme2016", "RG_CSP", "BPP_50_75_0.2_0.8_7.txt"), "bpplib_csp"),
            (os.path.join("delorme2016", "RG_CSP", "BPP_50_100_0.1_0.7_8.txt"), "bpplib_csp"),
            (os.path.join("delorme2016", "RG_CSP", "BPP_50_100_0.1_0.8_9.txt"), "bpplib_csp"),
            (os.path.join("delorme2016", "RG_CSP", "BPP_50_100_0.2_0.7_0.txt"), "bpplib_csp"),
            (os.path.join("delorme2016", "RG_CSP", "BPP_50_100_0.2_0.8_1.txt"), "bpplib_csp")]
    cutting_stock_main = os.path.join(
            "bazel-bin",
            "examples",
            "cutting_stock_main")
    for instance, instance_format in cutting_stock_data:
        instance_path = os.path.join(
                "data",
                "cutting_stock",
                instance)
        json_output_path = os.path.join(
                args.directory,
                "cutting_stock",
                instance + ".json")
        if not os.path.exists(os.path.dirname(json_output_path)):
            os.makedirs(os.path.dirname(json_output_path))
        command = (
                cutting_stock_main
                + "  --verbosity-level 1"
                + "  --input \"" + instance_path + "\""
                + " --format \"" + instance_format + "\""
                + "  --algorithm greedy"
                + " --internal-diving 1"
                + "  --output \"" + json_output_path + "\"")
        print(command)
        os.system(command)
        print()
    print()
    print()


if args.tests is None or "multiple-knapsack" in args.tests:
    print("Multiple knapsack problem")
    print("-------------------------")
    print()

    multiple_knapsack_data = [
            (os.path.join("fukunaga2011", "FK_1", "random10_60_1_1000_1_1.txt"), ""),
            (os.path.join("fukunaga2011", "FK_1", "random10_60_2_1000_1_2.txt"), ""),
            (os.path.join("fukunaga2011", "FK_1", "random10_100_3_1000_1_3.txt"), ""),
            (os.path.join("fukunaga2011", "FK_1", "random10_100_4_1000_1_4.txt"), ""),
            (os.path.join("fukunaga2011", "FK_1", "random12_48_1_1000_1_5.txt"), ""),
            (os.path.join("fukunaga2011", "FK_1", "random12_48_2_1000_1_6.txt"), ""),
            (os.path.join("fukunaga2011", "FK_1", "random15_45_3_1000_1_7.txt"), ""),
            (os.path.join("fukunaga2011", "FK_1", "random15_45_4_1000_1_8.txt"), ""),
            (os.path.join("fukunaga2011", "FK_1", "random15_75_1_1000_1_9.txt"), ""),
            (os.path.join("fukunaga2011", "FK_1", "random15_75_2_1000_1_10.txt"), ""),
            (os.path.join("fukunaga2011", "FK_1", "random30_60_3_1000_1_11.txt"), ""),
            (os.path.join("fukunaga2011", "FK_1", "random30_60_4_1000_1_12.txt"), "")]
    multiple_knapsack_main = os.path.join(
            "bazel-bin",
            "examples",
            "multiple_knapsack_main")
    for instance, instance_format in multiple_knapsack_data:
        instance_path = os.path.join(
                "data",
                "multiple_knapsack",
                instance)
        json_output_path = os.path.join(
                args.directory,
                "multiple_knapsack",
                instance + ".json")
        if not os.path.exists(os.path.dirname(json_output_path)):
            os.makedirs(os.path.dirname(json_output_path))
        command = (
                multiple_knapsack_main
                + "  --verbosity-level 1"
                + "  --input \"" + instance_path + "\""
                + " --format \"" + instance_format + "\""
                + "  --algorithm greedy"
                + " --internal-diving 1"
                + "  --output \"" + json_output_path + "\"")
        print(command)
        os.system(command)
        print()
    print()
    print()


if args.tests is None or "vehicle-routing-with-time-windows" in args.tests:
    print("Vehicle routing problem with time-windows")
    print("-----------------------------------------")
    print()

    vehicle_routing_with_time_windows_data = [
            (os.path.join("solomon1987", "C101.txt"), ""),
            (os.path.join("solomon1987", "C102.txt"), ""),
            (os.path.join("solomon1987", "C103.txt"), ""),
            (os.path.join("solomon1987", "R101.txt"), ""),
            (os.path.join("solomon1987", "R102.txt"), ""),
            (os.path.join("solomon1987", "R103.txt"), ""),
            (os.path.join("solomon1987", "RC101.txt"), ""),
            (os.path.join("solomon1987", "RC102.txt"), ""),
            (os.path.join("solomon1987", "RC103.txt"), "")]
    vehicle_routing_with_time_windows_main = os.path.join(
            "bazel-bin",
            "examples",
            "vehicle_routing_with_time_windows_main")
    for instance, instance_format in vehicle_routing_with_time_windows_data:
        instance_path = os.path.join(
                "data",
                "vehicle_routing_with_time_windows",
                instance)
        json_output_path = os.path.join(
                args.directory,
                "vehicle_routing_with_time_windows",
                instance + ".json")
        if not os.path.exists(os.path.dirname(json_output_path)):
            os.makedirs(os.path.dirname(json_output_path))
        command = (
                vehicle_routing_with_time_windows_main
                + "  --verbosity-level 1"
                + "  --input \"" + instance_path + "\""
                + " --format \"" + instance_format + "\""
                + "  --algorithm limited-discrepancy-search"
                + " --automatic-stop 1"
                + " --internal-diving 1"
                + "  --output \"" + json_output_path + "\"")
        print(command)
        os.system(command)
        print()
    print()
    print()
