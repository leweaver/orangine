from oe_bindings import Input, Quat, Entity
import math


class FirstPersonController:
    camera: Entity = None
    input: Input = None

    def __init__(self, camera: Entity, input_reader: Input):
        self.yaw = 0.0
        self.pitch = 0.0
        self.mouse_speed = 1.0 / 600.0
        self.camera = camera
        self.input_reader = input_reader

        self.update_camera()

    def tick(self):
        mouse_state = self.input_reader.get_mouse_state()
        if mouse_state.left == Input.MouseButtonState.Held or mouse_state.left == Input.MouseButtonState.Pressed:
            half_pi = 0.5 * math.pi
            two_pi = 2.0 * math.pi
            self.yaw += -mouse_state.delta_position.x * two_pi * self.mouse_speed;
            self.pitch = min(half_pi,
                             max(-half_pi, self.pitch + mouse_state.delta_position.y * two_pi * self.mouse_speed));

            self.update_camera()

    def update_camera(self):
        if not self.camera:
            return
        quat = Quat.rotation_y(self.yaw) * Quat.rotation_x(-self.pitch)
        self.camera.rotation = quat