import gnu.io.CommPortIdentifier;
import gnu.io.NoSuchPortException;
import gnu.io.PortInUseException;
import gnu.io.SerialPort;
import gnu.io.UnsupportedCommOperationException;

import java.awt.MouseInfo;
import java.awt.Point;
import java.awt.Robot;
import java.awt.event.InputEvent;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Enumeration;

import javax.swing.JOptionPane;

/**
 * Class for modeling reports from the GloveMouse
 * 
 * @author Tudor
 * 
 */
class MouseReport {
	private static final byte LEFT_CLICK = 0;
	private static final byte RIGHT_CLICK = 1;
	private static final byte SCROLL_CLICK = 2;

	byte buttons;
	byte x;
	byte y;
	byte wheel;

	public MouseReport() {
	}

	public MouseReport(byte buttons, byte x, byte y, byte wheel) {
		this.buttons = buttons;
		this.x = x;
		this.y = y;
		this.wheel = wheel;
	}

	public boolean leftClick() {
		return (buttons & (1 << LEFT_CLICK)) != 0;
	}

	public boolean rightClick() {
		return (buttons & (1 << RIGHT_CLICK)) != 0;
	}

	public boolean scrollClick() {
		return (buttons & (1 << SCROLL_CLICK)) != 0;
	}

	@Override
	public String toString() {
		return buttons + " " + x + " " + y + " " + wheel;
	}

}

/**
 * Class for exiting the application
 * 
 * @author Tudor
 * 
 */
class GloveMouseExitHelper extends Thread {
	private GloveMouseHelper mainClass;

	public GloveMouseExitHelper(GloveMouseHelper mainClass) {
		this.mainClass = mainClass;
	}

	@Override
	public void run() {
		JOptionPane.showMessageDialog(null, "Press OK to exit application");
		mainClass.quit();
	}
}

/**
 * Class that makes the GloveMouse work even with INADEQUATE bluetooth modules
 * :P
 * 
 * @author Tudor
 * 
 */
public class GloveMouseHelper {
	private volatile boolean done = false;

	/**
	 * Returns an array containing the names of available ports
	 * 
	 * @return an array containing the names of all(?) available COM ports
	 */
	@SuppressWarnings("unchecked")
	private static String[] getAvailablePorts() {
		ArrayList<String> ports = new ArrayList<String>();
		String[] tmp = new String[0];

		Enumeration<CommPortIdentifier> portEnum = CommPortIdentifier
				.getPortIdentifiers();
		while (portEnum.hasMoreElements())
			ports.add(portEnum.nextElement().getName());

		return ports.toArray(tmp);
	}

	/**
	 * Tries to open the communications port specified by portName
	 * 
	 * @param portName
	 *            the name of the port to be opened
	 * @return a SerialPort object or null if errors occurred
	 * 
	 * @throws NoSuchPortException
	 * @throws PortInUseException
	 * @throws UnsupportedCommOperationException
	 */
	private static SerialPort connect(String portName)
			throws NoSuchPortException, PortInUseException,
			UnsupportedCommOperationException {
		CommPortIdentifier portIdentifier = CommPortIdentifier
				.getPortIdentifier(portName);

		if (!portIdentifier.isCurrentlyOwned()) {
			SerialPort serialPort = (SerialPort) portIdentifier.open(
					"GloveMouseHelper", 2000);
			serialPort.setSerialPortParams(115200, SerialPort.DATABITS_8,
					SerialPort.STOPBITS_1, SerialPort.PARITY_NONE);
			serialPort.disableReceiveTimeout();
			serialPort.disableReceiveThreshold();
			serialPort.disableReceiveFraming();

			return serialPort;
		}

		return null;
	}

	/**
	 * Tells the main loop to exit. Ought to be called from another thread
	 */
	public void quit() {
		this.done = true;
	}

	/**
	 * Reads from the the input stream and assembles a MouseReport
	 * 
	 * @param input
	 *            the InputStream from which to read data
	 * @return a MouseReport containing newly acquired data or null if errors
	 *         occurred
	 */
	private static MouseReport getNewMouseReport(InputStream input) {
		byte buttons = 0, x = 0, y = 0, wheel = 0;

		System.out.print("Waiting for new report...");

		try {
			// wait until a start sequence ("TV") is received
			while (true) {
				byte tmp = (byte) input.read();

				if (tmp == -1) {
					System.out.println("no new report!");
					return null;
				}
				if ((tmp == 'T') && (input.read() == 'V'))
					break;
			}

			// receive the actual report
			System.out.print("receiving...");
			if ((buttons = (byte) input.read()) == -1) {
				System.out.println("report error!");
				return null;
			}
			if ((x = (byte) input.read()) == -1) {
				System.out.println("report error!");
				return null;
			}
			if ((y = (byte) input.read()) == -1) {
				System.out.println("report error!");
				return null;
			}
			if ((wheel = (byte) input.read()) == -1) {
				System.out.println("report error!");
				return null;
			}
		} catch (IOException e) {
			e.printStackTrace();
		}

		System.out.println("got it!");
		return new MouseReport(buttons, x, y, wheel);
	}

	/**
	 * Does all the work for GloveMouseHelper application
	 */
	public void work() {
		try {
			// connect to a COM port
			// find all available COM ports
			String[] ports = getAvailablePorts();

			// if no ports are available we obviously have nothing to do
			if (ports.length == 0) {
				JOptionPane.showMessageDialog(null, "No available COM ports",
						"Error", JOptionPane.ERROR_MESSAGE);
				return;
			}

			// display a list from which the user will choose the desired port
			String portName = (String) JOptionPane.showInputDialog(null,
					"Select COM port:", null, JOptionPane.PLAIN_MESSAGE, null,
					ports, ports[0]);
			SerialPort commPort = null;
			MouseReport report;

			// if no port was selected exit the application
			if (portName == null)
				return;

			// connect to selected COM port
			try {
				System.out.println("Connecting to " + portName);
				commPort = connect(portName);
			} catch (Exception e) {
				String message = portName + " : ";
				String[] tmp = e.getClass().getName().split("[.]");
				message += tmp[tmp.length - 1];

				JOptionPane.showMessageDialog(null, message, "Error",
						JOptionPane.ERROR_MESSAGE);

				return;
			}

			// helpers
			boolean leftClickFlag = false, rightClickFlag = false;
			Robot robot = new Robot();
			Point mousePosition;
			(new GloveMouseExitHelper(this)).start();

			// as far as this thread is concerned it will forever wait for
			// MouseReports
			while (!done) {
				report = getNewMouseReport(commPort.getInputStream());

				if (report != null) {
					System.out.println(report.toString());

					// check if scrolling
					if (report.leftClick() && report.rightClick()) {
						robot.mouseWheel(report.y);
					} else {
						// move the mouse as you see fit
						if (report.x != 0 || report.y != 0) {
							mousePosition = MouseInfo.getPointerInfo()
									.getLocation();

							robot.mouseMove(mousePosition.x + report.x,
									mousePosition.y + report.y);
						}
						
						if (report.leftClick()) {
							if (leftClickFlag) {
								leftClickFlag = false;
								robot.mousePress(InputEvent.BUTTON1_DOWN_MASK);
							}
						} else {
							if(!leftClickFlag) {
							leftClickFlag = true;
							robot.mouseRelease(InputEvent.BUTTON1_DOWN_MASK);
							}
						}

						if (report.rightClick()) {
							if (rightClickFlag) {
								rightClickFlag = false;
								robot.mousePress(InputEvent.BUTTON3_DOWN_MASK);
							}
						} else {
							if(!rightClickFlag) {
								rightClickFlag = true;
								robot.mouseRelease(InputEvent.BUTTON3_DOWN_MASK);
							}
						}
					}
				}
			} //while

			// close COM port and exit
			if (commPort != null)
				commPort.close();
			System.out.println("\nExiting");
			return;
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/**
	 * Main function for the GloveMouseHelper application
	 * 
	 * @param args
	 *            nothing. USELESS
	 */
	public static void main(String[] args) {
		(new GloveMouseHelper()).work();
	}
}
