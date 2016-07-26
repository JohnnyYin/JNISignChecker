package me.yinzhong.jnisignchecker;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // 这里加载so, so中可能包含核心逻辑, 即使这里被反编译去掉了。功能也是无法正常使用的。
        Jni.hi();
    }
}
