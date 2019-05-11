import sys
import requests


def main():
    p = "http://{}:{}/api/v1".format(sys.argv[1], sys.argv[2])
    u = "{}/election/list".format(p)
    r = requests.get(u)
    # print(r.json())

    u = "{}/election/dc2019/status".format(p)
    h = {
        "X-Election-Debug": "8192",
        "X-Election-Auth": b"A\xd3\xb5\x41\xfd=L\x88\xb0\xcc\x19\xf7rY\x01\xec\xd5\xc6\xde\x44k\xa5\xc5&\xd5\x10U\xfb\x88\xbc\x0e",
    }

    r = requests.get(u, headers=h)
    if r.status_code != 200:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
