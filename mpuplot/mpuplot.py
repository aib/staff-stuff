import math
import sys

import pyqtgraph

class MpuPlotter:
	def __init__(self, plot_length):
		self.plot_length = plot_length

		self.widget = pyqtgraph.GraphicsLayoutWidget(title="MPU Plotter")

		self.tracked = {
			'ax':   { 'components': 1, 'title': "Accel X",   'color': 'r', 'pos': (3, 0) },
			'ay':   { 'components': 1, 'title': "Accel Y",   'color': 'g', 'pos': (3, 1) },
			'az':   { 'components': 1, 'title': "Accel Z",   'color': 'b', 'pos': (3, 2) },
			'axy':  { 'components': 2, 'title': "Accel XY",  'color': 'y', 'pos': (4, 2) },
			'axz':  { 'components': 2, 'title': "Accel XZ",  'color': 'm', 'pos': (4, 1) },
			'ayz':  { 'components': 2, 'title': "Accel YZ",  'color': 'c', 'pos': (4, 0) },
			'axyz': { 'components': 3, 'title': "Accel XYZ", 'color': 'w', 'pos': (5, 1) },
			'gx':   { 'components': 1, 'title': "Gyro X",    'color': 'r', 'pos': (0, 0) },
			'gy':   { 'components': 1, 'title': "Gyro Y",    'color': 'g', 'pos': (0, 1) },
			'gz':   { 'components': 1, 'title': "Gyro Z",    'color': 'b', 'pos': (0, 2) },
			'gxy':  { 'components': 2, 'title': "Gyro XY",   'color': 'y', 'pos': (1, 2) },
			'gxz':  { 'components': 2, 'title': "Gyro XZ",   'color': 'm', 'pos': (1, 1) },
			'gyz':  { 'components': 2, 'title': "Gyro YZ",   'color': 'c', 'pos': (1, 0) },
			'gxyz': { 'components': 3, 'title': "Gyro XYZ",  'color': 'w', 'pos': (2, 1) },
		}

		self.plots = {}
		for k, v in self.tracked.items():
			plot = pyqtgraph.PlotItem(title=v['title'])
			plot.disableAutoRange()
			yrange = (-1, +1) if v['components'] == 1 else (0, math.sqrt(v['components']))
			plot.setRange(xRange=(0, self.plot_length), yRange=yrange)
			self.plots[k] = plot

		self.curves = { k: self.plots[k].plot(pen=v['color']) for k, v in self.tracked.items() }
		self.plot_data = { k: [0] * self.plot_length for k in self.tracked.keys() }

		# The order plots are added and updated seems to affect performance
		self.update_order = list(map(lambda k_v: k_v[0], sorted(self.tracked.items(), key=lambda k_v: k_v[1]['pos'])))

		for k in self.update_order:
			self.widget.addItem(self.plots[k], row=self.tracked[k]['pos'][0], col=self.tracked[k]['pos'][1])

		self._update_data()

		self.widget.showMaximized()

	def put_data(self, ax, ay, az, gx, gy, gz):
		keyed = {
			'ax': ax,
			'ay': ay,
			'az': az,
			'gx': gx,
			'gy': gy,
			'gz': gz,

			'axy': math.sqrt(ax*ax + ay*ay),
			'axz': math.sqrt(ax*ax + az*az),
			'ayz': math.sqrt(ay*ay + az*az),
			'axyz': math.sqrt(ax*ax + ay*ay + az*az),

			'gxy': math.sqrt(gx*gx + gy*gy),
			'gxz': math.sqrt(gx*gx + gz*gz),
			'gyz': math.sqrt(gy*gy + gz*gz),
			'gxyz': math.sqrt(gx*gx + gy*gy + gz*gz)
		}

		for k, v in keyed.items():
			self.plot_data[k] = self.plot_data[k][1:] + [v]

		self._update_data()

	def _update_data(self):
		for k in self.update_order:
			self.curves[k].setData(self.plot_data[k])

def main():
	app = pyqtgraph.Qt.QtGui.QApplication(sys.argv)

	pyqtgraph.setConfigOptions(antialias=True)

	mp = MpuPlotter(100)

	def updater():
		import time
		sine = lambda p: lambda t: math.sin(math.tau * t / p)

		fs = [
			sine(2),
			sine(3),
			sine(5),
			sine(2),
			sine(3),
			sine(5)
		]

		start = time.monotonic()
		while True:
			t = time.monotonic() - start
			data = tuple(map(lambda f: f(t), fs))
			mp.put_data(*data)
			time.sleep(.05)
	import threading
	threading.Thread(target=updater, daemon=True).start()

	app.exec_()

if __name__ == '__main__':
	main()

