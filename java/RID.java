package bplustrees;

public class RID {

	private int pageNum;
	private int slotNum;

	public RID() {
		pageNum = 0;
		slotNum = 0;
	}

	public RID(int pageNum, int slotNum) {
		this.pageNum = pageNum;
		this.slotNum = slotNum;
	}

	@Override
	public boolean equals(Object o) {
		return false;
	}

	public int GetPageNum() {
		return pageNum;
	}

	public int GetSlotNum() {
		return slotNum;
	}
}
