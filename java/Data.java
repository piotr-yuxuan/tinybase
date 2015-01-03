package bplustrees;

public class Data extends Object {

	public static final int length = 10;

	private Value value;

	public Data() {
		value = null;
	}

	public Data(Value value) {
		this.value = value;
	}

	public Value getValue() {
		return value;
	}

	public int getLength() {
		return length;
	}
}
