import csv
from pathlib import Path
CELL_CSV_PATH = Path("../../../2024_CellData/")
INPUT_FILENAME = "computed_ir.csv"
OUTPUT_FILENAME = "sorted_cells.csv"
cell_irs = []
PARALLEL_CELLS = 6
with open(CELL_CSV_PATH / INPUT_FILENAME, 'r') as r_f:
    r_f = r_f.readlines()[1:]
    reader = csv.reader(r_f)
    for row in reader:
        cell_number = row[0]
        cell_IR = row[1]
        cell_irs.append([int(cell_number), abs(float(cell_IR))])

#breakpoint()
cell_irs.sort(key=lambda k: k[1])
average_ir = sum(k[1] for k in cell_irs)/len(cell_irs)
print(f"Average Internal Resistance Desired: {average_ir}")

# Naive Cell IR groupings
cell_groupings = []
index = 0
while index < len(cell_irs)/2:
    local_cell_group = []
    for i in range(index, index+int(PARALLEL_CELLS/2)):
        local_cell_group.append(cell_irs[i])
    for j in range(len(cell_irs)-1-index-int(PARALLEL_CELLS/2), len(cell_irs)-1-index):
        local_cell_group.append(cell_irs[j])
    cell_groupings.append(local_cell_group)
    index = index + int(PARALLEL_CELLS/2)
print(cell_groupings[0])

cell_ir_distribution = []
for index, cell_group in enumerate(cell_groupings):
    group_mean = sum(cell[1] for cell in cell_group)/len(cell_group)
    print(f"Group {index}, IR: {group_mean}")


# Log data
with open(CELL_CSV_PATH / OUTPUT_FILENAME, 'w') as out_f:
    fieldnames = ["Cell Group Index", "Internal Resistance"]
    for cell in range(1,PARALLEL_CELLS+1):
        fieldnames.append(f"Cell {cell}")
    csv_writer = csv.DictWriter(out_f, fieldnames=fieldnames)
    csv_writer.writeheader()
    for index, cell_group in enumerate(cell_groupings):
        if len(cell_group) != PARALLEL_CELLS:
            print(f"Incomplete Cell Group {index} has {len(cell_group)} parallel cells")
            continue
        group_mean = sum(cell[1] for cell in cell_group)/len(cell_group)
        struct = {fieldnames[0]: index, fieldnames[1]: group_mean}
        for index, cell in enumerate(cell_group):
            struct[f"Cell {index+1}"] = cell[0]
        csv_writer.writerow(struct)
    
#with open(CELL_CSV_PATH / OUTPUT_FILENAME, 'w') as out_f:
#    reader = csv.reader(r_f)
    
