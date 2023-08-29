import csv
import os
from pathlib import Path
import re

CURRENT_COLUMN = "IShunt [A]"
VOLTAGE_COLUMN = "Cell Voltage [V]"

STATE_ZERO_CURRENT = 0
STATE_WAIT_FOR_FULL_CURRENT = 1
STATE_FULL_CURRENT = 2
STATE_DONE = 3

ZERO_I_LOW_THRESHOLD = 0.1
HIGH_I_THRESHOLD = 8.9
DECLINE_I_THRESHOLD = 8.0

CELL_OCV_COUNTER = 25 

CELL_CSV_PATH = Path("../../../2024_CellData/")
OUTPUT_FILENAME = "computed_ir.csv"
all_files = os.listdir(CELL_CSV_PATH)
sorted_files = {}

for f_name in all_files:
    m = re.match("(\w)+-(\d)+_(\d)+-(\d)+-(\d)+_cell_(\d+)\.csv", f_name)
    if m != None:
        sorted_files[int(m.group(6))] = f_name

sorted_files = dict(sorted(sorted_files.items()))

with open(CELL_CSV_PATH / OUTPUT_FILENAME, 'w') as out_f:
    fieldnames = ["Cell Index", "Internal Resistance"]
    csv_writer = csv.DictWriter(out_f, fieldnames=fieldnames)
    csv_writer.writeheader()
    for ir_idx, csv_fname in sorted_files.items():
        with open(CELL_CSV_PATH / csv_fname) as csv_f:
            state = 0
            cnt = 0
            mean_sum = 0
            csv_reader = csv.DictReader(csv_f)
            ocv = 0
            filtered_ir = 0
            for row in csv_reader:
                if state == STATE_ZERO_CURRENT:
                    if abs(float(row[CURRENT_COLUMN])) < ZERO_I_LOW_THRESHOLD and cnt < CELL_OCV_COUNTER:
                        mean_sum += float(row[VOLTAGE_COLUMN])
                        cnt += 1
                    else:
                        ocv = mean_sum / cnt
                    if float(row[CURRENT_COLUMN]) >= HIGH_I_THRESHOLD:
                        state = STATE_FULL_CURRENT
                        cnt = 0
                        mean_sum = 0
                elif state == STATE_FULL_CURRENT:
                    if float(row[CURRENT_COLUMN]) >= HIGH_I_THRESHOLD:
                        inst_ir = (ocv - float(row[VOLTAGE_COLUMN])) / float(row[CURRENT_COLUMN])
                        mean_sum += inst_ir
                        cnt += 1
                        filtered_ir = mean_sum / cnt
                    if float(row[CURRENT_COLUMN]) <= DECLINE_I_THRESHOLD:
                        state = STATE_DONE
                elif state == STATE_DONE:
                    print(filtered_ir)
                    csv_writer.writerow({fieldnames[0]: ir_idx, fieldnames[1]: filtered_ir})
                    break




        

#with CELL_CSV_PATH.open() as f:
#    csv_reader = csv.DictReader(f)

