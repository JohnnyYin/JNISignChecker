#include <jni.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include "android/log.h"

// 日志tag
#define LOG_TAG "SignChecker"

// debug开关
#define DEBUG true

// 使用编译脚本控制的宏
#ifndef CHECK_SIGN
#define CHECK_SIGN false
#endif

// 应用的正式签名
#define SIGN_HEX "95D866E6B6EC18A80A041D89E65A4CA3"

const char *const JNIREG_CLASS = "me/yinzhong/jnisignchecker/Jni";

#define ALOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))

extern "C" {

JNIEXPORT jstring JNICALL
hi(JNIEnv *env, jclass type) {
    return env->NewStringUTF("hello");
}

/**
 * byte转换为16进制字符串
 */
void byteToHexStr(const unsigned char *source, char *dest, int sourceLen) {
    short i;
    unsigned char highByte, lowByte;

    for (i = 0; i < sourceLen; i++) {
        highByte = source[i] >> 4;
        lowByte = source[i] & 0x0f;

        highByte += 0x30;

        if (highByte > 0x39)
            dest[i * 2] = highByte + 0x07;// 大写字母
        else
            dest[i * 2] = highByte;// 数字

        lowByte += 0x30;
        if (lowByte > 0x39)
            dest[i * 2 + 1] = lowByte + 0x07;
        else
            dest[i * 2 + 1] = lowByte;
    }
    *(dest + sourceLen * 2) = '\0';// 在末尾补\0
    return;
}

/**
 * 初始化, 校验签名
 */
void checkSign(JNIEnv *env) {
    if (!CHECK_SIGN) {
        if (DEBUG)
            ALOGD("skip check sign.");
        return;
    }
    int result = JNI_FALSE;
    jclass clsApplication = env->FindClass("me/yinzhong/jnisignchecker/DemoApplication");
    // "()Lcom/johnnyyin/ndkdemo/DemoApplication;"
    jmethodID midGetInstance = env->GetStaticMethodID(clsApplication, "getInstance",
                                                      "()Lme/yinzhong/jnisignchecker/DemoApplication;");
    jobject application = env->CallStaticObjectMethod(clsApplication, midGetInstance);

    jmethodID midGetPackageManager = env->GetMethodID(clsApplication, "getPackageManager",
                                                      "()Landroid/content/pm/PackageManager;");
    jmethodID midGetPackageName = env->GetMethodID(clsApplication, "getPackageName",
                                                   "()Ljava/lang/String;");
    jmethodID midGetPackageInfo = env->GetMethodID(
            env->FindClass("android/content/pm/PackageManager"), "getPackageInfo",
            "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");

    jobject packageManager = env->CallObjectMethod(application, midGetPackageManager);
    jstring packageName = (jstring) env->CallObjectMethod(application, midGetPackageName);

    jobject packageInfo = env->CallObjectMethod(packageManager, midGetPackageInfo, packageName,
                                                0x00000040);

    jfieldID fidSignatures = env->GetFieldID(env->FindClass("android/content/pm/PackageInfo"),
                                             "signatures",
                                             "[Landroid/content/pm/Signature;");
    jobjectArray signatures = (jobjectArray) env->GetObjectField(packageInfo, fidSignatures);
    int length = env->GetArrayLength(signatures);
    for (int i = 0; i < length; i++) {
        jobject signature = env->GetObjectArrayElement(signatures, i);

        // localSignature.toByteArray()
        jmethodID midToByteArray = env->GetMethodID(env->GetObjectClass(signature), "toByteArray",
                                                    "()[B");
        jobject obj_sign_byte_array = env->CallObjectMethod(signature, midToByteArray);

        //      MessageDigest localMessageDigest = MessageDigest.getInstance("MD5");
        jclass clsMessageDigest = env->FindClass("java/security/MessageDigest");
        jmethodID midMessageDigestGetInstance = env->GetStaticMethodID(clsMessageDigest,
                                                                       "getInstance",
                                                                       "(Ljava/lang/String;)Ljava/security/MessageDigest;");
        jobject objMd5 = env->CallStaticObjectMethod(clsMessageDigest, midMessageDigestGetInstance,
                                                     env->NewStringUTF("md5"));
        // localMessageDigest.reset();
        jmethodID midReset = env->GetMethodID(clsMessageDigest, "reset", "()V");
        env->CallVoidMethod(objMd5, midReset);

        //      localMessageDigest.update(localSignature.toByteArray());
        //tem_class = (*env)->GetObjectClass(env, obj_md5);
        jmethodID midUpdate = env->GetMethodID(clsMessageDigest, "update", "([B)V");
        env->CallVoidMethod(objMd5, midUpdate, obj_sign_byte_array);
        // localMessageDigest.digest()
        jmethodID midDigest = env->GetMethodID(clsMessageDigest, "digest", "()[B");
        // 这个是md5以后的byte数组，现在只要将它转换成16进制字符串，就可以和之前的比较了
        jbyteArray objArraySign = (jbyteArray) env->CallObjectMethod(objMd5, midDigest);
        //      // 这个就是签名的md5值
        //      String str2 = toHex(localMessageDigest.digest());
        jsize arrayLength = env->GetArrayLength(objArraySign);
        jbyte *byteArrayElements = env->GetByteArrayElements(objArraySign, JNI_FALSE);
        char *charResult = (char *) malloc((size_t) arrayLength * 2 + 1);// 后面+1预留\0结束符
        // 将byte数组转换成16进制字符串, 这里强转为unsigned char *, 因为上面返回的byte数组是没符号的。
        byteToHexStr((unsigned char *) byteArrayElements, charResult, arrayLength);

        int cmpResult = strcmp(charResult, SIGN_HEX);

        if (DEBUG) {
            ALOGD(env->GetStringUTFChars(env->NewStringUTF(charResult), JNI_FALSE));
        }

        // release
        env->ReleaseByteArrayElements(objArraySign, byteArrayElements, JNI_ABORT);
        free(charResult);

        result = (cmpResult == 0 ? JNI_TRUE : JNI_FALSE);
    }

    if (result == JNI_TRUE) {
        if (DEBUG)
            ALOGD("checkSign success.");
    } else {
        if (DEBUG)
            ALOGD("checkSign fail.");
        exit(0);
    }
}

// Java和JNI函数的绑定表
JNINativeMethod method_table[] = {
        {"hi", "()Ljava/lang/String;", (void *) hi},//绑定
};

// 注册native方法到java中
int registerNativeMethods(JNIEnv *env) {
    jclass clazz;
    clazz = env->FindClass(JNIREG_CLASS);
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, method_table, sizeof(method_table) / sizeof(method_table[0])) <
        0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


void init(JNIEnv *env) {
    ::checkSign(env);
    if (::registerNativeMethods(env) == JNI_FALSE) {
        exit(0);
    };
}

/**
 * 成功则返回JNI版本号，失败返回-1.
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    assert(env != NULL);

    // 执行初始化操作
    ::init(env);

    /* 注册成功，返回JNI版本号 */
    return JNI_VERSION_1_6;
}

/**
 * 反注册给类成员函数
 */
void unregisterNatives(JNIEnv *env) {
    if (NULL == env) {
        return;
    }

    jclass clazz = NULL;
    clazz = env->FindClass(JNIREG_CLASS);
    if (env->ExceptionCheck() || clazz == NULL) {
        env->ExceptionClear();
        clazz = NULL;
        return;
    }
    env->UnregisterNatives(clazz);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
}

/**
 * 解除注册
 */
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6))
        return;

    unregisterNatives(env);
    return;
}

}