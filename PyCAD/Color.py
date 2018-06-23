from OpenGL.GL import *

class HeeksColor:
	def __init__(self, red = 0, green = 0, blue = 0, colorref = None):
		if colorref != None:
			self.red   = ( colorref      & 0xff)
			self.green = ((colorref>> 8) & 0xff)
			self.blue  = ((colorref>>16) & 0xff)
		else:
			self.red = red
			self.green = green
			self.blue = blue

	def COLORREF_color(self):
		return self.red | (self.green << 8) | (self.blue << 16)

	def best_black_or_white(self):
		if self.red + self.green + self.blue > 0x17e: return HeeksColor(0, 0, 0)
		return HeeksColor(255, 255, 255)

	def glColor(self):
		glColor3ub(self.red, self.green, self.blue)

	def glClearColor(self, alpha):
		glClearColor(float(self.red)/255, float(self.green)/255, float(self.blue)/255, alpha)
