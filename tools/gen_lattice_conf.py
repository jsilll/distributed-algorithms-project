import argparse
from random import choice

def main():
    # parse command line arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('--directory', required=True)
    parser.add_argument('--values', required=True, type=int)
    parser.add_argument('--proposals', required=True, type=int)
    parser.add_argument('--processes', required=True, type=int)
    parser.add_argument('--max_length', required=True, type=int)
    args = parser.parse_args()

    max_length = min(args.max_length, args.values)

    # generate config files
    for i in range(1, args.processes + 1):
        with open(f"{args.directory}/{i}.lattice-agreement.config", "w") as f:
            f.write(f"{args.proposals} {max_length} {args.values}\n")
            for _ in range(args.proposals):
                proposal = set()
                possible = set(range(args.values))
                length = choice(range(1, max_length + 1))
                for _ in range(length):
                    val = choice(list(possible))
                    proposal.add(val)
                    possible.remove(val)
                f.write(" ".join([str(val) for val in proposal]) + "\n")

if __name__ == "__main__":
    main()