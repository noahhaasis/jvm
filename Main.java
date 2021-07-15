class Main {
  public static void main(String[] args) {
    System.out.println(fac());
    System.out.println("Hi there");
  }

  public static int fac() {
    int n = 5;
    if (n == 0) return 0;

    int res = 1;
    for (int i = 2; i <= n; i++) {
      res *= i;
    } 
    return res;
  }
}
