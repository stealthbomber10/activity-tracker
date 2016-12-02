/*
 * @author Trent Andraka
 * @author Austin Stover
 * Rudimentary Fitbit with SparkFun
 * Arduino, tracks steps, amount of sleep,
 * and temperature. On Java/PC side, takes
 * in data, calculates steps, and displays
 * data.
 */

package fitbit.steps;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.*;

public class MsgReceiver
{
	final private DataInputStream dataStream;
	//final private PrintStream ps;
	//final private ViewInputStream psp = new ViewInputStream(ps);

	static final private int MAGIC_NUM = '!';

	static final private int DEBUG_STRING = 0x30; //UTF encoded String
	static final private int ERROR_STRING = 0x31; //UTF encoded String
	static final private int TIMESTAMP = 0x32; //4-Byte Integer
	static final private int ACCEL_X = 0x33; //4-Byte Float
	static final private int ACCEL_Y = 0x34; //4-Byte Float
	static final private int ACCEL_Z = 0x35; //4-Byte Float
	static final private int NUM_STEPS = 0x36; //2 byte integer
	static final private int CONVERTED_TEMP = 0x37; //4-Byte Float in degrees C
	static final private int SLEEP_TIME = 0x38; //4-Byte Integer
	static final private int RESET_STEPS = 0x39;
	static final private double ACCEL_THRESHOLD = 1.5; //use if calculating steps with threshold

	private ArrayList<Float> accelx = new ArrayList<Float>();
	private ArrayList<Float> accely = new ArrayList<Float>();
	private ArrayList<Float> accelz = new ArrayList<Float>();
	private ArrayList<Integer> times = new ArrayList<Integer>();

	private String sleepTime;
	private String convTemp;
	private String timeStamp = "1";
	private String itemz;
	private String numSteps = "0";

	public MsgReceiver(InputStream in) throws IOException
	{
		dataStream = new DataInputStream(new assign8.steps.ViewInputStream(in));
	}

	public void run() throws IOException, InterruptedException
	{
		while(true)
		{
			if(dataStream.available() > 0)
			{
				if(dataStream.readByte() == MAGIC_NUM)
				{
					byte type = dataStream.readByte();
					String inputData = readData(type);
					System.out.println(inputData);
				}
			}
		}
	}

	String readData(byte type) throws IOException
	{
		switch(type)
		{
			case DEBUG_STRING:
				System.out.print("Debug: ");
				return dataStream.readUTF();

			case ERROR_STRING:
				System.out.print("Error: ");
				return "!!!!! " + dataStream.readUTF();

			case TIMESTAMP:
				System.out.print("Timestamp: ");
				timeStamp = Integer.toString(dataStream.readInt());
				return timeStamp;

			case ACCEL_X:
				System.out.print("X-Acceleration: ");
				String itemx = Float.toString(readNextFourBytes());
				accelx.add(Float.parseFloat(itemx));
				return itemx;

			case ACCEL_Y:
				System.out.print("Y-Acceleration: ");
				String itemy = Float.toString(readNextFourBytes());
				accely.add(Float.parseFloat(itemy));
				return itemy;

			case ACCEL_Z:
				System.out.print("Z-Acceleration: ");
				itemz = Float.toString(readNextFourBytes());
				accelz.add(Float.parseFloat(itemz));
				return itemz;

			case NUM_STEPS:
				System.out.print("Step Count: ");
				numSteps = Integer.toString(calcSteps());
				return numSteps;

			case CONVERTED_TEMP:
				System.out.print("Converted Temp: ");
				convTemp = Float.toString(readNextFourBytes());        //dataStream.readFloat());

				return convTemp;

			case SLEEP_TIME:
				System.out.print("Sleep Time: ");
				sleepTime = Integer.toString(dataStream.readInt());        //dataStream.readFloat());


				return sleepTime;

			case RESET_STEPS:
				reset();
				return "STEP COUNT RESET";

			default:
				return null;
		}
	}

	private float readNextFourBytes() throws IOException
	{
		byte[] fourByte = new byte[4];
		for(int i = 0; i < 4; i++)
		{
			fourByte[i] = dataStream.readByte();
		}
		float num = ByteBuffer.wrap(fourByte).order(ByteOrder.LITTLE_ENDIAN).getFloat();
		return num;
	}

	public void drawAccel() {

	}

	public int calcSteps() {
		int size = accelz.size();
		int steps = 0;
		//COMMENT OUT IF USING PRE-SET MEAN TO MEASURE PEAKS (POTENTIALLY MORE ACCCURATE, BUT MORE VOLATILE)
		float mean = 0;
		float sum = 0;
		if (size < 3) {
			return 0;
		}
		for (int i = 0; i < size; i++) {
			sum += accelz.get(i);
		}
		mean = sum / size;
		for (int i = 1; i < size - 1; i++) {
			if ((accelz.get(i) > accelz.get(i - 1))
					&& (accelz.get(i) > accelz.get(i + 1))
					&& (accelz.get(i) > mean)) {		//Put ACCEL_THRESHOLD in place of mean if using pre-set determination
				steps += 1;
			}
		}
		return steps;

	}

	public void reset() {
		for (int k = 0; k < accelz.size(); k++) {
			accelz.remove(k);
		}
	}

	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
        try
        {
            SerialComm s = new SerialComm();
            s.connect("COM3"); // Adjust this to be the right port
            InputStream in = s.getInputStream();
            MsgReceiver msgr = new MsgReceiver(in);
            msgr.run();
        }
        catch ( Exception e )
        {
            e.printStackTrace();
            System.exit(1);
        }

	}

}
