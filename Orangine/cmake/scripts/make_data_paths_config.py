import json
import sys
config = dict(
    data_path_overrides={} if (len(sys.argv) == 1) else dict(item.split('=') for item in sys.argv[1].split(';'))
)
print(json.dumps(config, sort_keys=True, indent=2))