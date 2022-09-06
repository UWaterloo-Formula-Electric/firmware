#By default graph acceleration but with the -v flag plot velocity

import argparse
import csv
import math
import matplotlib.pyplot as plt

def getMagnitudeOfVector(vector):
    sumOfSquareOfComponents = 0
    for component in vector:
        sumOfSquareOfComponents += component**2
    return math.sqrt(sumOfSquareOfComponents)

def filter(src_file):
    data = {}
    data["magnitude of acceleration [ms^-2]"] = []
    data["magnitude of velocity [ms^-1]"] = []
    with open(src_file, 'r') as f:
        next(f) # skip the first row as it is headers
        reader = csv.reader(f)
        
        velocityX = 0
        velocityY = 0
        velocityZ = 0
        time = 0.0
        try:
            line = 1
            for row in reader:
                accel_vector = row[1][1:len(row[1])-1].split(",")           #Array of strings
                accel_vector = [float(x.strip()) for x in accel_vector]     #3D vector of floats
                data["magnitude of acceleration [ms^-2]"].append((time, getMagnitudeOfVector(accel_vector)))
                velocityX += accel_vector[0] * 1/6 #We get measurements at 6Hz
                velocityY += accel_vector[1] * 1/6 #We get measurements at 6Hz
                velocityZ += accel_vector[2] * 1/6 #We get measurements at 6Hz
                data["magnitude of velocity [ms^-1]"].append((time, getMagnitudeOfVector([velocityX, velocityY, velocityZ])))
                line += 1
                time += 1/6
        except:
            print("error on line {}".format(line))
    return data

def graph(args):
    data_dict = filter(args.src_file)
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
    parser.add_argument('-ymax', type=float)
    parser.add_argument('-ymin', type=float)
    args = parser.parse_args()
    graph(args)

if __name__ == "__main__":
    main()
