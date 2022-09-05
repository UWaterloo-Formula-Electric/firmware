#By default graph acceleration but with the -v flag plot velocity

import argparse
import csv
import math

def getMagnitudeOfAcceleration(vector):
    return math.sqrt(float(vector[0].strip())**2 + float(vector[1].strip())**2 + float(vector[0].strip())**2)

def filter(src_file, plotVelocity):
    filtered_data = []
    with open(src_file, 'r') as f:
        next(f) # skip the first row of headers
        reader = csv.reader(f)
        try:
            velocity = 0
            line = 1
            for row in reader:
                acceleration = getMagnitudeOfAcceleration(row[1][1:len(row[1])-1].split(","))
                if plotVelocity:
                    #integrate acceleration
                    filtered_data.append(acceleration * 1/6) # we get measurements at 6 hertz
                else:
                    #plot acceleration directly
                    filtered_data.append(acceleration)
                line += 1
        except:
            print("error on line {}".format(line))
    return filtered_data

def graph(data, args):
    import matplotlib.pyplot as plt
    data_dict = {}
    if args.velocity:
        sig_name = "velocity"
    else:
        sig_name = "acceleration"
    data_dict[sig_name] = []
    time = 0
    for row in data:
        data_dict[sig_name].append((float(time), float(row)))
        time += 1/6 # 6 hertz
    fig, ax = plt.subplots()
    for signal in data_dict:
        ax.plot(*zip(*data_dict[signal]), label=signal, marker='.')
    if args.ymax is not None:
        plt.ylim(top=args.ymax)
    if args.ymin is not None:
        plt.ylim(bottom=args.ymin)
    ax.legend()
    plt.xlabel("Time")
    plt.show()

def main():
    parser = argparse.ArgumentParser(description="Process IMU log")
    parser.add_argument('src_file', help="IMU log file (csv) to be read in")
    parser.add_argument('-v', '--velocity', action='store_true', help="Graph velocity")
    parser.add_argument('-ymax', type=float)
    parser.add_argument('-ymin', type=float)
    args = parser.parse_args()
    filtered_data = filter(args.src_file, args.velocity)
    graph(filtered_data, args)

if __name__ == "__main__":
    main()
