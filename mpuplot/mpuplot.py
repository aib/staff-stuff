import math
import socketserver
import struct
import sys
import threading

import pyqtgraph

class MpuPlotter:
	class SignalContainer(pyqtgraph.Qt.QtCore.QObject):
		data_updated = pyqtgraph.Qt.QtCore.pyqtSignal()

	def __init__(self, plot_length):
		self.plot_length = plot_length
		self.signals = self.SignalContainer()

		self.widget = pyqtgraph.GraphicsLayoutWidget(title="MPU Plotter")
		self.signals.data_updated.connect(self._data_updated)

		self.tracked = {
			'ax':   { 'components': 1, 'title': "Accel X--", 'color': 'r', 'pos': (3, 0) },
			'ay':   { 'components': 1, 'title': "Accel -Y-", 'color': 'g', 'pos': (4, 0) },
			'az':   { 'components': 1, 'title': "Accel --Z", 'color': 'b', 'pos': (5, 0) },
			'axy':  { 'components': 2, 'title': "Accel XY-", 'color': 'y', 'pos': (5, 2) },
			'axz':  { 'components': 2, 'title': "Accel X-Z", 'color': 'm', 'pos': (4, 2) },
			'ayz':  { 'components': 2, 'title': "Accel -YZ", 'color': 'c', 'pos': (3, 2) },
			'axyz': { 'components': 3, 'title': "Accel XYZ", 'color': 'w', 'pos': (4, 1) },
			'gx':   { 'components': 1, 'title': "Gyro X--",  'color': 'r', 'pos': (0, 0) },
			'gy':   { 'components': 1, 'title': "Gyro -Y-",  'color': 'g', 'pos': (1, 0) },
			'gz':   { 'components': 1, 'title': "Gyro --Z",  'color': 'b', 'pos': (2, 0) },
			'gxy':  { 'components': 2, 'title': "Gyro XY-",  'color': 'y', 'pos': (2, 2) },
			'gxz':  { 'components': 2, 'title': "Gyro X-Z",  'color': 'm', 'pos': (1, 2) },
			'gyz':  { 'components': 2, 'title': "Gyro -YZ",  'color': 'c', 'pos': (0, 2) },
			'gxyz': { 'components': 3, 'title': "Gyro XYZ",  'color': 'w', 'pos': (1, 1) },
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

		self.signals.data_updated.emit()

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

		self.signals.data_updated.emit()

	def _data_updated(self, *args, **kwargs):
		for k in self.update_order:
			self.curves[k].setData(self.plot_data[k])

def main():
	app = pyqtgraph.Qt.QtGui.QApplication(sys.argv)

	pyqtgraph.setConfigOptions(antialias=True)

	mp = MpuPlotter(100)

	class _DataHandler(socketserver.DatagramRequestHandler):
		def handle(self):
			tof = lambda i: i / 32768. if i < 0 else i / 32767.
			sensor_data = self.rfile.read()
			unpacked = struct.unpack('>hhhhhhh', sensor_data)
			mp.put_data(
				tof(unpacked[0]), tof(unpacked[1]), tof(unpacked[2]),
				tof(unpacked[4]), tof(unpacked[5]), tof(unpacked[6])
			)

	threading.Thread(target=socketserver.UDPServer(('0.0.0.0', 12345), _DataHandler).serve_forever, daemon=True).start()

	app.exec_()

if __name__ == '__main__':
	main()

