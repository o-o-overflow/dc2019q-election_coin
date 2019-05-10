import sys
import requests


def main():
    u = "http://{}:{}/api/v1/election/list".format(sys.argv[1], sys.argv[2])
    r = requests.get(u)
    if r.status_code != 200:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
