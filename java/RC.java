package bplustrees;

public class RC {

	public static RC OK = new RC(0);
	public static RC EXCEPTION = new RC(1);
	public static RC ERROR = new RC(-1);

	private Integer value;

	public RC() {
		value = OK.toInt();
	}

	public RC(int value) {
		this.value = value;
	}

	public RC(Integer value) {
		this.value = value;
	}

	@Override
	public String toString() {
		return value.toString();
	}

	public int toInt() {
		return value.intValue();
	}
}
