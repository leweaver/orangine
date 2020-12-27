import sys
import oe
import logging

from io import StringIO

def enable_remote_debugging():
    import ptvsd
    ptvsd.enable_attach(address=('0.0.0.0', 3000), redirect_output=False)
    pass

class NativeHandler(logging.StreamHandler):
    def __init(self):
        # Init to stderr, so that if there is an error when logging, it is output there.
        logging.StreamHandler.__init__(self, sys.stderr)

    def emit(self, record):
        """
        Emit a record.

        If a formatter is specified, it is used to format the record.
        The record is then written to the appropriate native logging method.
        If exception information is present, it is formatted using
        traceback.print_exception and sent to stderr native method.
        If the stream has an 'encoding' attribute, it is used to determine
        how to do the output to the stream.
        """
        try:
            msg = self.format(record)
            if record.levelno >= logging.WARNING:
                oe.log_warning(msg)
            elif record.levelno >= logging.INFO:
                oe.log_info(msg)
            elif record.levelno >= logging.DEBUG:
                oe.log_debug(msg)
        except RecursionError:  # See issue logger 36272
            raise
        except Exception:
            self.handleError(record)

def _init_logger():
    logger = logging.getLogger()
    if oe.log_debug_enabled():
        logger.setLevel(logging.DEBUG)
    elif oe.log_info_enabled():
        logger.setLevel(logging.INFO)
    else:
        logger.setLevel(logging.WARNING)

    native_handler = NativeHandler()
    native_handler.setFormatter(logging.Formatter('[%(name)s] %(message)s'))
    native_handler.setLevel(logger.level)
    logger.addHandler(native_handler)

def _init_statics():
  oe.init_statics()
  
def init():
  _init_logger()
  _init_statics()

# Captures stdout and stderr. This is just a bucket for any output not using logger
# (which is handled by init_logger, above)
def reset_output_streams():
    if not sys.stdout:
        sys.stdout = StringIO()
        sys.stdout.flush()
    else:
        s = sys.stdout.getvalue()
        if oe.log_info_enabled() and len(s):
            for line in s.split("\\n"):
                oe.log_info(line)
        sys.stdout.seek(0)
        sys.stdout.truncate()

    if not sys.stderr:
        sys.stderr = StringIO()
        sys.stderr.flush()
    else:
        s = sys.stderr.getvalue()
        if oe.log_warning_enabled() and len(s):
            for line in s.split("\\n"):
                oe.log_warning(line)
        sys.stderr.seek(0)
        sys.stderr.truncate()