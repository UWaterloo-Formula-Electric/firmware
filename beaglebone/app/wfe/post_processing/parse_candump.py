import can
import cantools
import argparse
    
def main():
    parser = argparse.ArgumentParser(description="Process CAN log")
    parser.add_argument('src_file', help="CAN log file (csv) to be read in")
    parser.add_argument('--dbc', default="../../../../common/Data/2018CAR.dbc")
    args = parser.parse_args()

    db = cantools.database.load_file(args.dbc)
    with open(args.src_file, "r") as f:
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
                    print(f"{timestamp},{signal},{sig_data}")
            except:
                # Sometimes messages not in DBC 
                pass
            
if __name__ == "__main__":
    main()
