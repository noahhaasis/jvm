class Main {
  public static void main(String[] args) {
    int[] list = {1, 2, 3};
    fn(list);
  }

  public static int fn(int[] arr) {
    arr[2] = 4;
    return arr[1];
  }
}
