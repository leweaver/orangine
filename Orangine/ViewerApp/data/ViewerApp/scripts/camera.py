from logging import log, info
from oe import managers, get_scene, Input, Vector3, Quat
import math

class FirstPersonCamera:
  def __init__(self):
    self.yaw = math.pi
    self.pitch = 0.0
    self.mouse_speed = 1.0 / 600.0
    
    self.update_camera()

  def tick(self):
    mouse_state = managers.input.get_mouse_state()
    if mouse_state.left == Input.MouseButtonState.Held or mouse_state.left == Input.MouseButtonState.Pressed:

      half_pi = 0.5 * math.pi
      two_pi = 2.0 * math.pi
      self.yaw += -mouse_state.delta_position.x * two_pi * self.mouse_speed;
      self.pitch = min(half_pi, max(-half_pi, self.pitch + mouse_state.delta_position.y * two_pi * self.mouse_speed));

      self.update_camera()
      
  def update_camera(self):
    cam = get_scene().get_main_camera()
    if not cam:
      return
    quat = Quat.rotation_y(self.yaw) * Quat.rotation_x(-self.pitch)
    cam.rotation = quat
