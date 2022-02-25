from oe_bindings import runtime
from oeapp import camera_controller
import logging


def load_sample():
    logging.info('Loading sample')

    camera = runtime().scene_graph.instantiate("camera")
    ctrl = camera_controller.FirstPersonController(camera)


load_sample()
