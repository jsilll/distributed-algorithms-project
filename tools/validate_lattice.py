import argparse
from itertools import combinations

def get_possible(configs : list) -> set:
    possible = set()
    for config in configs:
        fconfig = open(config, "r")
        fconfig.readline()
        for line in fconfig:
            proposal = [int(n) for n in line.split(" ")]
            possible.update(proposal)
    return possible

def check_conf_out(config : str, output : str) -> bool:
    foutput = open(output, "r")
    fconfig = open(config, "r")
    fconfig.readline()
    for line in foutput:
        decided = set([int(n) for n in line.split(" ") if n != "\n"])
        proposed = set([int(n) for n in fconfig.readline().split(" ") if n != "\n"])
        for item in proposed:
            if item not in decided:
                return False
    return True

def check_out_possible(output : str, possible : set) -> bool:
    foutput = open(output, "r")
    for line in foutput:
        decided = set([int(n) for n in line.split(" ") if n != "\n"])
        for item in decided:
            if item not in possible:
                return False
    return True

def check_out_out(output1 : str, output2 : str) -> bool:
    foutput1 = open(output1, "r")
    foutput2 = open(output2, "r")
    for line1, line2 in zip(foutput1, foutput2):
        decided1 = set([int(n) for n in line1.split(" ") if n != "\n"])
        decided2 = set([int(n) for n in line2.split(" ") if n != "\n"])
        if len(decided1) == len(decided2) and decided1 != decided2:
            return False
        elif len(decided1) < len(decided2):
            for item in decided1:
                if item not in decided2:
                    return False
        else:
            for item in decided2:
                if item not in decided1:
                    return False
    return True

def main():
    # parse command line arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', nargs='+', required=True)
    parser.add_argument('--output', nargs='+', required=True)
    args = parser.parse_args()

    # 1. checking each config file against each output file
    for config, output in zip(args.config, args.output):
        print("Checking {} against {}".format(config, output))
        if  not check_conf_out(config, output):
            print("Validation failed!")
            exit(1)

    # 2. checking each output file against the set of possible values
    possible = get_possible(args.config)
    for output in args.output:
        print("Checking {} against possible values".format(output))
        if not check_out_possible(output, possible):
            print("Validation failed!")
            exit(1)

    # 3. checking each output file against each other
    for output1, output2 in combinations(args.output, 2):
        print("Checking {} against {}".format(output1, output2))
        if not check_out_out(output1, output2):
            print("Validation failed!")
            exit(1)

if __name__ == "__main__":
    main()