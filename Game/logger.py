"""Logger helper functions"""
import logging

def init():
    """Init logging"""
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)

    stream_handler = logging.StreamHandler()
    stream_handler.setLevel(logging.DEBUG)
    stream_handler.setFormatter(
        logging.Formatter('%(levelname)s:%(name)s:%(message)s')
    )
    logger.addHandler(stream_handler)

    file_handler = logging.FileHandler(filename='game.log', mode='w')
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(
        logging.Formatter('%(asctime)s %(levelname)s:%(name)s:%(message)s'))
    logger.addHandler(file_handler)
