import argparse
import os
from pathlib import Path
import subprocess as sub


def parse_args():
    def _validate_file(arg):
        f = Path(arg)
        if f.is_file():
            return f
        else:
            raise FileNotFoundError(arg)
    
    parser = argparse.ArgumentParser(description='Replay CAN messages')
    parser.add_argument('-c', type=str, help='CAN interface', required=True)
    parser.add_argument('-i', type=_validate_file, help='Log file with CAN messages from UWFE candump', required=True)
    return parser.parse_args()

def main():
    args = parse_args()
    os.system(f"cat {args.i} | awk '{{print $1\" \"$2\" \"$3\"#\"$5$6$7$8$9$10$11$12}}' | canplayer {args.c}=can1")

if __name__ == '__main__':
    main()