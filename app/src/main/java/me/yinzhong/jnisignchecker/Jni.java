package me.yinzhong.jnisignchecker;

public class Jni {
    static {
        System.loadLibrary("sign-checker");
    }

    public static native String hi();

}
