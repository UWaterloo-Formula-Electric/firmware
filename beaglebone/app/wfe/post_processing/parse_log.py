import argparse
import logging
import csv
import re
import codecs


def sort_csv(csv_reader, column, rev=False):
    return sorted(csv_reader, key = lambda row: row[column], reverse=rev)

def signal_filter(input_signal, signal_l, regex_l):
    if input_signal in signal_l:
        return True
    for r in regex_l:
        if re.search(r, input_signal) is not None:
            return True
    return False

def filter(src_file, signals_l, regex_l):
    filtered_data = []
    with open(src_file, 'r') as f:
        reader = csv.reader(f)
        sorted_csv = sort_csv(reader, 0)
        try:
            for row in sorted_csv:
                if signal_filter(row[1], signals_l, regex_l):
                    filtered_data.append(row)
        except:
            pass
    return filtered_data


def graph(data, args):
    import matplotlib.pyplot as plt
    data_dict = {}
    for row in data:
        sig_name = row[1]
        if sig_name in data_dict:
            data_dict[sig_name].append((float(row[0]), float(row[2])))
        else:
            data_dict[sig_name] = [(float(row[0]), float(row[2]))]
    fig, ax = plt.subplots()
    for signal in data_dict:
        ax.plot(*zip(*data_dict[signal]), label=signal)
    if args.ymax is not None:
        plt.ylim(top=args.ymax)
    if args.ymin is not None:
        plt.ylim(bottom=args.ymin)
    ax.legend()
    plt.xlabel("Time")
    plt.show()
    
def main():
    parser = argparse.ArgumentParser(description="Process CAN log")
    subparsers = parser.add_subparsers(dest="subparser")
    parser.add_argument('src_file', help="CAN log file (csv) to be read in")
    parser_g = subparsers.add_parser('graph', help="Graph the CAN signals")
    parser_f = subparsers.add_parser('filter', help="Filter out to only specified signals")
    parser.add_argument('-a', '--add', default=[], action="append", help="CAN signals to be included in analysis")
    parser.add_argument('-r', '--regex', default=[], action="append", help="CAN signal regex to be included in analysis")
    parser.add_argument('-o', '--output', help="Write output to file")
    parser_g.add_argument('-ymax', type=float)
    parser_g.add_argument('-ymin', type=float)
    args = parser.parse_args()
    if args.subparser == "filter":
        if len(args.add) == 0 and len(args.regex) == 0:
            logging.error("No CAN signals provided")
            return
        filtered_data = filter(args.src_file, args.add, args.regex)
        if args.output is None:
            print(filtered_data)
        else:
            with open(args.output, 'w') as f:
                for line in filtered_data:
                    f.write(','.join(line) + '\n')
    elif args.subparser == "graph":
        if len(args.add) == 0 and len(args.regex) == 0:
            logging.error("No CAN signals provided")
            return
        filtered_data = filter(args.src_file, args.add, args.regex)
        graph(filtered_data, args)




if __name__ == "__main__":
    main()
