"""Build Nitro G3D resource dictionaries and their Patricia search nodes."""

import struct


def name_bit(name, position):
    byte = position // 8
    return (name[byte] >> (position % 8)) & 1 if byte < len(name) else 0


def differing_bit(first, second):
    for byte in range(max(len(first), len(second)) - 1, -1, -1):
        left = first[byte] if byte < len(first) else 0
        right = second[byte] if byte < len(second) else 0
        if left != right:
            return byte * 8 + (left ^ right).bit_length() - 1
    return 0


def search_nodes(names):
    """Return (tested bit, left, right, record index) nodes for 16-byte names."""
    if len(set(names)) != len(names) or len(names) > 255:
        raise ValueError("Nitro dictionary needs unique names and at most 255 entries")
    keys = [name.encode("ascii") for name in names]
    if any(not key or len(key) > 16 for key in keys):
        raise ValueError("Nitro dictionary names must be 1-16 ASCII bytes")
    nodes = [[127, 0, 0, 0]]
    for entry, key in enumerate(keys):
        parent, child = 0, nodes[0][1]
        while nodes[parent][0] > nodes[child][0]:
            parent = child
            child = nodes[child][2 if name_bit(key, nodes[child][0]) else 1]
        leaf = keys[nodes[child][3]] if child else b""
        bit = differing_bit(key, leaf)
        parent, child = 0, nodes[0][1]
        while nodes[parent][0] > nodes[child][0] and nodes[child][0] > bit:
            parent = child
            child = nodes[child][2 if name_bit(key, nodes[child][0]) else 1]
        index = len(nodes)
        if name_bit(key, bit):
            node = [bit, child, index, entry]
        else:
            node = [bit, index, child, entry]
        nodes[parent][1 if nodes[parent][1] == child else 2] = index
        nodes.append(node)
    order = [0]

    def visit(parent):
        for child in nodes[parent][1:3]:
            if nodes[child][0] < nodes[parent][0] and child not in order:
                order.append(child)
                visit(child)

    visit(0)
    if len(order) != len(nodes):
        raise ValueError("Nitro dictionary tree is disconnected")
    index = {old: new for new, old in enumerate(order)}
    return [(nodes[old][0], index[nodes[old][1]], index[nodes[old][2]], nodes[old][3])
            for old in order]


def replace_names(data, offset, names):
    count = data[offset + 1]
    if len(names) != count:
        raise ValueError("Nitro dictionary name count changed")
    nodes = search_nodes(names)
    for index, node in enumerate(nodes):
        data[offset + 8 + 4 * index:offset + 12 + 4 * index] = bytes(node)
    names_offset = offset + 16 + 8 * count
    for index, name in enumerate(names):
        encoded = name.encode("ascii")
        data[names_offset + 16 * index:names_offset + 16 * (index + 1)] = encoded.ljust(16, b"\0")


def build_dictionary(names, records):
    if len(names) != len(records) or any(len(record) != 4 for record in records):
        raise ValueError("Nitro material binding dictionary needs four-byte records")
    count = len(names)
    nodes = search_nodes(names)
    result = bytearray(struct.pack("<BBHHH", 0, count, 16 + 24 * count, 8, 12 + 4 * count))
    for node in nodes:
        result.extend(bytes(node))
    result.extend(struct.pack("<HH", 4, 4 + 4 * count))
    for record in records:
        result.extend(record)
    for name in names:
        result.extend(name.encode("ascii").ljust(16, b"\0"))
    return bytes(result)
