package bplustrees;

public class Value implements Comparable<Value> {

	private int value;

	private Value() {
	}

	public Value(Object o) {
		this.value = o.hashCode();
	}

	/**
	 * a.compareTo(b). Returns sign of a - b.
	 * 
	 * @return -1 a < b
	 * @return 0 égalité
	 * @return 1 a > b
	 * 
	 */
	@Override
	public int compareTo(Value o) {
		return (o.value == this.value) ? 0 : (o.value > this.value) ? -1 : 1;
	}

}
