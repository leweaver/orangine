import sys
import engine

from io import StringIO

def enable_remote_debugging():
  # import ptvsd
  # ptvsd.enable_attach(address=('0.0.0.0', 3000), redirect_output=False)
  pass

def reset_output_streams():
    if not sys.stdout:
        sys.stdout = StringIO()
        sys.stdout.flush()
    else:
        s = sys.stdout.getvalue()
        if engine.log_info_enabled() and len(s):
            for line in s.split("\\n"):
                engine.log_info(line);
        sys.stdout.seek(0)
        sys.stdout.truncate()

    if not sys.stderr:
        sys.stderr = StringIO()
        sys.stderr.flush()
    else:
        s = sys.stderr.getvalue()
        if engine.log_warning_enabled() and len(s):
            for line in s.split("\\n"):
                engine.log_warning(line);
        sys.stderr.seek(0)
        sys.stderr.truncate()