import math
import sys

import pyqtgraph

class MpuPlotter:
	def __init__(self, plot_length):
		self.plot_length = plot_length

		self.widget = pyqtgraph.GraphicsLayoutWidget(title="MPU Plotter")

		self.tracked = {
			'ax': { 'components': 1, 'title': "Accel X", 'color': 'r' },
			'ay': { 'components': 1, 'title': "Accel Y", 'color': 'g' },
			'az': { 'components': 1, 'title': "Accel Z", 'color': 'b' },
			'axy': { 'components': 2, 'title': "Accel XY", 'color': 'y' },
			'axz': { 'components': 2, 'title': "Accel XZ", 'color': 'm' },
			'ayz': { 'components': 2, 'title': "Accel YZ", 'color': 'c' },
			'axyz': { 'components': 3, 'title': "Accel XYZ", 'color': 'w' },
			'gx': { 'components': 1, 'title': "Gyro X", 'color': 'r' },
			'gy': { 'components': 1, 'title': "Gyro Y", 'color': 'g' },
			'gz': { 'components': 1, 'title': "Gyro Z", 'color': 'b' },
			'gxy': { 'components': 2, 'title': "Gyro XY", 'color': 'y' },
			'gxz': { 'components': 2, 'title': "Gyro XZ", 'color': 'm' },
			'gyz': { 'components': 2, 'title': "Gyro YZ", 'color': 'c' },
			'gxyz': { 'components': 3, 'title': "Gyro XYZ", 'color': 'w' },
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

		self.widget.addItem(self.plots['gx'])
		self.widget.addItem(self.plots['gy'])
		self.widget.addItem(self.plots['gz'])
		self.widget.nextRow()
		self.widget.addItem(self.plots['gyz'])
		self.widget.addItem(self.plots['gxz'])
		self.widget.addItem(self.plots['gxy'])
		self.widget.nextRow()
		self.widget.addViewBox()
		self.widget.addItem(self.plots['gxyz'])
		self.widget.addViewBox()
		self.widget.nextRow()
		self.widget.addItem(self.plots['ax'])
		self.widget.addItem(self.plots['ay'])
		self.widget.addItem(self.plots['az'])
		self.widget.nextRow()
		self.widget.addItem(self.plots['ayz'])
		self.widget.addItem(self.plots['axz'])
		self.widget.addItem(self.plots['axy'])
		self.widget.nextRow()
		self.widget.addViewBox()
		self.widget.addItem(self.plots['axyz'])
		self.widget.addViewBox()

		self._update_data()

		self.widget.show()

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
		for k, curve in self.curves.items():
			curve.setData(self.plot_data[k])

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

