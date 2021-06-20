import sys


def _main(argv):
    print(argv)


def main():
    sys.exit(_main(sys.argv[1:]))


if __name__ == "__main__":
    main()
