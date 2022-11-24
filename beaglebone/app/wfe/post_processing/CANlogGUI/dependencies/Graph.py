from readline import set_pre_input_hook
import cantools
import matplotlib.pyplot as plt
import csv
import numpy as np

def toDict(src_file):
     db = cantools.database.load_file("../../../../../common/Data/2018CAR.dbc")
     signals = {}
     with open(src_file, "r") as f:
          lines = f.readlines()
          for line in lines:
               try:
                    words = line.split()
                    timestamp = float(words[0].strip("()"))
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

def graph(checked_data_dict, args):
    # if (somekey in args):
        # plt.add_setting
    fig, ax = plt.subplots()
    for signal in checked_data_dict:
        ax.plot(*zip(*checked_data_dict[signal]), label=signal)

    #check if users has inputed any constraints on graph
    if (args['MaxXInput'] != ''):
        MaxXInput = int(args['MaxXInput'])
        plt.xlim(right=MaxXInput)
    if (args['MinXInput'] != ''):
        MinXInput = int(args['MinXInput'])
        plt.xlim(left=MinXInput)
    if (args['MaxYInput'] != ''):
        MaxYInput = int(args['MaxYInput'])
        plt.ylim(top=MaxYInput)
    if (args['MinYInput'] != ''):
        MinYInput = int(args['MinYInput'])
        plt.ylim(bottom=MinYInput)
    ax.legend()
    plt.xlabel("Time")

    plt.show()

import json
from cantools.database.can.signal import NamedSignalValue

def serialize(msg):
        """ JSON encode our topic and message into a multipart message """
        # if isinstance(msg, NamedSignalValue):
        #     msg = msg.name()
        invalid_signals = []
        for signal_k in msg.keys():
            for key_pair in msg[signal_k]:
                timestamp, value = key_pair
                if isinstance(value, NamedSignalValue):
                    # print(signal_k)
                    invalid_signals.append(signal_k)
        for sig in invalid_signals:
            if(sig in msg):
                del msg[sig]
        # print(msg)
        msg_string = json.dumps(msg)
        return msg_string

def logToJsonDict(logFilePath, dbcFile):
    if dbcFile == "":
        db = cantools.database.load_file("../../../../../common/Data/2018CAR.dbc")
    else:
        db = cantools.database.load_file(dbcFile)
    signals = {}
    with open(logFilePath, "r") as f:
        lines = f.readlines()
        for line in lines:
            try:
                words = line.split()
                timestamp = float(words[0].strip("()"))
                iden = int(words[2], 16)
                data = [int(d, 16) for d in words[4:]]
                decoded_data = db.decode_message(
                    iden,
                    data
                )
                for signal in decoded_data:
                    sig_data = decoded_data[signal]
                    # print(f"{timestamp},{signal},{sig_data}")
                    if signal in signals:
                        signals[signal].append((timestamp, sig_data))
                    else:
                        signals[signal] = [(timestamp, sig_data)]
            except:
                # Sometimes messages not in DBC 
                pass
    # print(signals)
    signals = serialize(signals)
    return signals

def csvToDict(csvFilePath):
    with open(csvFilePath, "r") as csv_file:
        lines = csv.reader(csv_file)
        data_dict = {}
        for line in lines:
            timestamp = line[0]
            signal_name = line[1]
            value = line[2]
            if (value == "Wait_System_Up"):
                value = -1
            if (signal_name in data_dict.keys()):
                data_dict[signal_name].append((timestamp,value))
            else:
                data_dict[signal_name] = [(timestamp,value)]
    return data_dict
