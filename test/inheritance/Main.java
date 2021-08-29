class Main extends ParentClass {
  public static void main(String[] args) {
    getParentField();
  }

  public static int getParentField() {
    return (new Main()).num;
  }
}
