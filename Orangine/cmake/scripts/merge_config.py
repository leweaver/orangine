import json
import sys
import io
from collections.abc import Sequence, Mapping

def dict_merge(dct, merge_dct):
    """ Recursive dict merge. Inspired by :meth:``dict.update()``, instead of
    updating only top-level keys, dict_merge recurses down into dicts nested
    to an arbitrary depth, updating keys. The ``merge_dct`` is merged into
    ``dct``. Additionally, will append list items (which may result in duplicate entries).
    :param dct: dict onto which the merge is executed
    :param merge_dct: dct merged into dct
    :return: None
    """
    for k in merge_dct:
        if (not k in dct):
            dct[k] = merge_dct[k]
        elif (isinstance(dct[k], dict) and isinstance(merge_dct[k], Mapping)):
            dict_merge(dct[k], merge_dct[k])
        elif (isinstance(dct[k], list) and isinstance(merge_dct[k], Sequence)):
            dct[k].extend(merge_dct[k])
        else:
            dct[k] = merge_dct[k]

merged_config = {}
for i in range(1, len(sys.argv)):
    config = json.load(open(sys.argv[i]))
    dict_merge(merged_config, config)

print(json.dumps(merged_config, sort_keys=True, indent=2))