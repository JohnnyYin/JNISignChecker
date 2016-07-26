package me.yinzhong.jnisignchecker;

import android.app.Application;
import android.content.Context;
import android.util.Log;

public class DemoApplication extends Application {
    private static DemoApplication sInstance;

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        sInstance = this;
    }

    public static DemoApplication getInstance() {
        Log.d("TR", "DemoApplication.getInstance:" + Log.getStackTraceString(new Throwable()));
        return sInstance;
    }
}
