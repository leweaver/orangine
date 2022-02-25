from ...OeApp.oe_app_bindings import SampleScene
from ...OeApp.scripts.camera_controller import FirstPersonController


class SceneLoader:
    """An ABC that is called from the native side."""
    def __init__(self):
        pass

    def load(self, sample_scene: SampleScene):
        pass


class BasicScene(SceneLoader):
    camera_controller: FirstPersonController = None

    def __init__(self):
        super().__init__()

    def load(self, sample_scene: SampleScene):
        camera_ent = sample_scene.scene_graph.instantiate("Camera")
        self.camera_controller = FirstPersonController(camera_ent)
