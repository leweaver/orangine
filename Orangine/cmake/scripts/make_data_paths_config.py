import json
import sys

try:
    config = dict(
        data_path_overrides={} if (len(sys.argv) == 1) else dict(item.split('=') for item in sys.argv[1].split(';'))
    )
    print(json.dumps(config, sort_keys=True, indent=2))
except Exception:
    print("Failed to make data paths config. Called with: " + " ".join(sys.argv), file=sys.stderr)
    raise
