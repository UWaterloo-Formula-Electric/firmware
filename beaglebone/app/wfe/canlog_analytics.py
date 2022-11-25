import os
import csv
import argparse
from pathlib import Path
import matplotlib.pyplot as plt


def parse_log(directory=None, file=None, make_plot=False):
    if directory is not None:
        files = os.listdir(directory)
    else:
        files = [Path(file)]
    for filename in files:
        # print(filename)
        if not str(filename).endswith(".csv"):
            continue
        if directory is not None:
            filename = Path(directory) / filename
        with open(filename) as csvfile:
            timestamps, timestamp_diff = [], []
            csvreader = csv.reader(csvfile)
            for row in csvreader:
                try:
                    timestamps.append(float(row[0]))
                except ValueError as e:
                    pass

            for idx in range(1, len(timestamps)-1):
                timestamp_diff.append((timestamps[idx] - timestamps[idx-1])*1000)

            avg_delta_ms = (sum(timestamp_diff)/len(timestamp_diff))
            max_delta_ms = max(timestamp_diff)

            print("Parsed {} lines from {}".format(len(timestamps), str(filename).split('\\')[-1]))
            print(f"Timestamp Deltas - AVG: {avg_delta_ms:.1f}ms\t MAX: {max_delta_ms:.1f}ms\n")

            if make_plot:
                fig = plt.figure(str(filename).split('\\')[-1])
                plt.plot(timestamp_diff)
                plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='Process canlog csv files to get analytics.'
                    ' Can Pass in either a single file using --filename, or a whole folder using --directory'
    )
    parser.add_argument('--directory', type=str)
    parser.add_argument('--filename', type=str)
    parser.add_argument('--plot', nargs='?', const=True, default=False)

    args = parser.parse_args()

    parse_log(directory=args.directory, file=args.filename, make_plot=args.plot)


if __name__ == "__main__":
    main()
