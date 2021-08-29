class Native {
  native void sayHello();

  static {
    System.loadLibrary("native");
  }

  public static void main(String[] args) {
    new Native().sayHello();
  }
}
