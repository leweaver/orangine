from logging import log, info
from oe import managers, Input
import math

class OrbitCamera:
  def __init__(self):
    self.yaw = math.pi
    self.pitch = 0.0
    self.mouse_speed = 1.0 / 600.0

  def tick(self):
    mouse_state = managers.input.getMouseState()
    if mouse_state.left == Input.MouseButtonState.Held or mouse_state.left == Input.MouseButtonState.Pressed:
      two_pi = 2.0 * math.pi
      self.yaw += -mouse_state.delta_position.x * two_pi * self.mouse_speed;
      self.pitch += mouse_state.delta_position.y * two_pi * self.mouse_speed;
      
      info(f'delta.xy=({mouse_state.delta_position.x}, {mouse_state.delta_position.y}) yaw={self.yaw} pitch={self.pitch}')
