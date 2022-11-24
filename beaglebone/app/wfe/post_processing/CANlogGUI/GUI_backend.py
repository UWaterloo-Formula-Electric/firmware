import argparse
import csv
import re
import cantools

def toDict(src_file):
     db = cantools.database.load_file("../../../../common/Data/2018CAR.dbc")
     signals = {}
     with open(src_file, "r") as f:
          lines = f.readlines()
          for line in lines:
               try:
                    words = line.split()
                    timestamp = words[0].strip("()")
                    iden = int(words[2], 16)
                    data = [int(d, 16) for d in words[4:]]
                    decoded_data = db.decode_message(
                         iden,
                         data
                    )
                    for signal in decoded_data:
                         sig_data = decoded_data[signal]
                         if signal in signals:
                              signals[signal].append((timestamp, sig_data))
                         else:
                              signals[signal] = [(timestamp, sig_data)]
               except:
                    # Sometimes messages not in DBC 
                    pass
     return signals

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

def graph(data_dict, args):
     import matplotlib.pyplot as plt
     fig, ax = plt.subplots()
     for signal in data_dict:
          ax.plot(*zip(*data_dict[signal]), label=signal)
     ax.legend()
     plt.xlabel("Time")
     plt.show()

def main():
     parser = argparse.ArgumentParser(description="Process CAN log")
     parser.add_argument('src_file', help="CAN log file (csv) to be read in")
     args = parser.parse_args()
     data_dict = toDict(args.src_file)
     graph(data_dict, args)

if __name__ == "__main__":
     main()