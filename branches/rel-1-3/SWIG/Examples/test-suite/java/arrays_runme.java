
// This is the arrays runtime testcase. It ensures that a getter and a setter has 
// been produced for array members and that they function as expected. It is a
// pretty comprehensive test for all the array typemaps.

import arrays.*;

public class arrays_runme {

  static {
    try {
	System.loadLibrary("arrays");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code library failed to load. See the chapter on Dynamic Linking Problems in the SWIG Java documentation for help.\n" + e);
      System.exit(1);
    }
  }

  public static void main(String argv[]) {

    // Check array member variables
    ArrayStruct as = new ArrayStruct();

    // Create arrays for all the array types that ArrayStruct can handle
    String array_c = "X";
    byte[] array_sc = {10, 20};
    short[] array_uc = {101, 201};
    short[] array_s = {1002, 2002};
    int[] array_us = {1003, 2003};
    int[] array_i = {1004, 2004};
    long[] array_ui = {1005, 2005};
    int[] array_l = {1006, 2006};
    long[] array_ul = {1007, 2007};
    long[] array_ll = {1008, 2008};
    float[] array_f = {1009.1f, 2009.1f};
    double[] array_d = {1010.2f, 2010.2f};
    int[] array_enum = {arrays.Three, arrays.Four};

    long[] array_enumpointers = {arrays.newintpointer(), arrays.newintpointer()};
    arrays.setintfrompointer(array_enumpointers[0], arrays.One);
    arrays.setintfrompointer(array_enumpointers[1], arrays.Five);

    SimpleStruct[] array_struct={new SimpleStruct(), new SimpleStruct()};
    array_struct[0].setDouble_field(222.333);
    array_struct[1].setDouble_field(444.555);

    long[] array_ipointers = {arrays.newintpointer(), arrays.newintpointer()};
    arrays.setintfrompointer(array_ipointers[0], 567);
    arrays.setintfrompointer(array_ipointers[1], 890);

    // Now set the array members and check that they have been set correctly
    as.setArray_c(array_c);
    check_string(array_c, as.getArray_c());

    as.setArray_sc(array_sc);
    check_byte_array(array_sc, as.getArray_sc());

    as.setArray_uc(array_uc);
    check_short_array(array_uc, as.getArray_uc());

    as.setArray_s(array_s);
    check_short_array(array_s, as.getArray_s());

    as.setArray_us(array_us);
    check_int_array(array_us, as.getArray_us());

    as.setArray_i(array_i);
    check_int_array(array_i, as.getArray_i());

    as.setArray_ui(array_ui);
    check_long_array(array_ui, as.getArray_ui());

    as.setArray_l(array_l);
    check_int_array(array_l, as.getArray_l());

    as.setArray_ul(array_ul);
    check_long_array(array_ul, as.getArray_ul());

    as.setArray_ll(array_ll);
    check_long_array(array_ll, as.getArray_ll());

    as.setArray_f(array_f);
    check_float_array(array_f, as.getArray_f());

    as.setArray_d(array_d);
    check_double_array(array_d, as.getArray_d());

    as.setArray_enum(array_enum);
    check_int_array(array_enum, as.getArray_enum());

    as.setArray_ipointers(array_ipointers);
    check_ipointers_array(array_ipointers, as.getArray_ipointers());

    as.setArray_enumpointers(array_enumpointers);
    check_ipointers_array(array_enumpointers, as.getArray_enumpointers());

    as.setArray_struct(array_struct);
    check_struct_array(array_struct, as.getArray_struct());

    as.setArray_structpointers(array_struct);
    check_struct_array(array_struct, as.getArray_structpointers());
 }

  // Functions to check that the array values were set correctly
  public static void check_string(String original, String checking) {
    if (!checking.equals(original)) {
      System.err.println("Runtime test failed. checking = [" + checking + "]");
      System.exit(1);
    }
  }
  public static void check_byte_array(byte[] original, byte[] checking) {
    for (int i=0; i<original.length; i++) {
      if (checking[i] != original[i]) {
        System.err.println("Runtime test failed. checking[" + i + "]=" + checking[i]);
        System.exit(1);
      }
    }
  }
  public static void check_short_array(short[] original, short[] checking) {
    for (int i=0; i<original.length; i++) {
      if (checking[i] != original[i]) {
        System.err.println("Runtime test failed. checking[" + i + "]=" + checking[i]);
        System.exit(1);
      }
    }
  }
  public static void check_int_array(int[] original, int[] checking) {
    for (int i=0; i<original.length; i++) {
      if (checking[i] != original[i]) {
        System.err.println("Runtime test failed. checking[" + i + "]=" + checking[i]);
        System.exit(1);
      }
    }
  }
  public static void check_long_array(long[] original, long[] checking) {
    for (int i=0; i<original.length; i++) {
      if (checking[i] != original[i]) {
        System.err.println("Runtime test failed. checking[" + i + "]=" + checking[i]);
        System.exit(1);
      }
    }
  }
  public static void check_float_array(float[] original, float[] checking) {
    for (int i=0; i<original.length; i++) {
      if (checking[i] != original[i]) {
        System.err.println("Runtime test failed. checking[" + i + "]=" + checking[i]);
        System.exit(1);
      }
    }
  }
  public static void check_double_array(double[] original, double[] checking) {
    for (int i=0; i<original.length; i++) {
      if (checking[i] != original[i]) {
        System.err.println("Runtime test failed. checking[" + i + "]=" + checking[i]);
        System.exit(1);
      }
    }
  }
  public static void check_struct_array(SimpleStruct[] original, SimpleStruct[] checking) {
    for (int i=0; i<original.length; i++) {
      if (checking[i].getDouble_field() != original[i].getDouble_field()) {
        System.err.println("Runtime test failed. checking[" + i + "].double_field=" + checking[i].getDouble_field());
        System.exit(1);
      }
    }
  }
  public static void check_ipointers_array(long[] original, long[] checking) {
    for (int i=0; i<original.length; i++) {
        int checking_val = arrays.getintfrompointer(checking[i]);
        int original_val = arrays.getintfrompointer(original[i]);
      if (checking_val != original_val) {
        System.err.println("Runtime test failed. checking[" + i + "]=" + checking_val);
        System.exit(1);
      }
    }
  }
}
