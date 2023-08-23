/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _TYPE_CHANGE_UTILS
#define _TYPE_CHANGE_UTILS
#include <jni.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <assert.h>
#include <string>
#include <vector>

using namespace android;
using ::android::sp;
using ::android::RefBase;

#define BOOLEAN_CLASS "java/lang/Boolean"
#define LONG_CLASS "java/lang/Long"
#define INTEGER_CLASS "java/lang/Integer"

struct TypeChangeUtils {
    TypeChangeUtils();
    ~TypeChangeUtils();
    static bool getValue(JNIEnv *env, jobject obj, jfieldID fieldId, bool* value);
    static bool getValue(JNIEnv *env, jobject obj, jfieldID fieldId, int* value);
    static bool getValue(JNIEnv *env, jobject obj, jfieldID fieldId, long* value);
    static bool getBaseValue(JNIEnv *env, jobject integerObj, int* value);
    static jbyteArray getJNIArray(JNIEnv *env, char *data, int size);
    static jintArray getJNIArray(JNIEnv *env, int *data, int size);
    static bool getIntVector(JNIEnv *env, jintArray intArray, std::vector<int> *int_vector);
    static bool getCharVector(JNIEnv *env, jbyteArray byteArray, std::vector<char> *char_vector);
private:
    static jclass getValueClass(JNIEnv *env, jobject valueObject, const char *name);
};

#endif/*_TYPE_CHANGE_UTILS*/

