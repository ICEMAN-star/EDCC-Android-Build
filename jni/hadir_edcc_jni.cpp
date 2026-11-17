
#include <jni.h>

#include <exception>
#include <limits>
#include <string>
#include <vector>

#include "edcc/c_api.h"

namespace {

constexpr size_t STATUS_BUFFER_SIZE = 1024;

void throwJavaException(
        JNIEnv* env,
        const char* exceptionClass,
        const std::string& message) {

    jclass cls = env->FindClass(exceptionClass);

    if (cls != nullptr) {
        env->ThrowNew(cls, message.c_str());
        env->DeleteLocalRef(cls);
    }
}

void throwNativeError(
        JNIEnv* env,
        const char* message) {

    throwJavaException(
            env,
            "java/lang/IllegalStateException",
            message
    );
}

void throwStatusError(
        JNIEnv* env,
        const char* status) {

    unsigned char code =
            static_cast<unsigned char>(status[0]);

    if (code != 0) {
        std::string message = status + 1;

        if (message.empty()) {
            message = "EDCC operation failed.";
        }

        throwNativeError(env, message.c_str());
    }
}

bool validEncoderId(int encoderId) {
    return encoderId >= 0;
}

} // namespace


extern "C" {


JNIEXPORT jint JNICALL
Java_com_hadir_EdccNative_nativeCreateEncoder(
        JNIEnv* env,
        jclass,
        jint imageSize,
        jint gaborKernelSize,
        jint laplaceKernelSize,
        jint gaborDirections) {

    if (imageSize < 0 || imageSize > 255 ||
        gaborKernelSize < 0 || gaborKernelSize > 255 ||
        laplaceKernelSize < 0 || laplaceKernelSize > 255 ||
        gaborDirections < 0 || gaborDirections > 255) {

        throwJavaException(
                env,
                "java/lang/IllegalArgumentException",
                "Invalid EDCC configuration values."
        );

        return -1;
    }

    try {

        char status[STATUS_BUFFER_SIZE] = {};

        int encoderId = new_encoder_with_config(
                static_cast<unsigned char>(imageSize),
                static_cast<unsigned char>(gaborKernelSize),
                static_cast<unsigned char>(laplaceKernelSize),
                static_cast<unsigned char>(gaborDirections),
                status
        );

        if (status[0] != 0) {
            throwStatusError(env, status);
            return -1;
        }

        if (!validEncoderId(encoderId)) {
            throwNativeError(
                    env,
                    "EDCC encoder creation failed."
            );

            return -1;
        }

        return static_cast<jint>(encoderId);

    } catch (const std::exception& e) {

        throwNativeError(env, e.what());
        return -1;

    } catch (...) {

        throwNativeError(
                env,
                "Unknown error while creating EDCC encoder."
        );

        return -1;
    }
}


JNIEXPORT jlong JNICALL
Java_com_hadir_EdccNative_nativeGetCodeSize(
        JNIEnv* env,
        jclass,
        jint encoderId) {

    if (!validEncoderId(encoderId)) {
        throwJavaException(
                env,
                "java/lang/IllegalArgumentException",
                "Invalid EDCC encoder ID."
        );

        return 0;
    }

    try {

        unsigned long size =
                get_size_of_code_buffer_required(
                        static_cast<int>(encoderId)
                );

        if (size == 0 ||
            size > static_cast<unsigned long>(
                    std::numeric_limits<jlong>::max())) {

            throwNativeError(
                    env,
                    "Invalid EDCC code buffer size."
            );

            return 0;
        }

        return static_cast<jlong>(size);

    } catch (const std::exception& e) {

        throwNativeError(env, e.what());
        return 0;

    } catch (...) {

        throwNativeError(
                env,
                "Unable to get EDCC code buffer size."
        );

        return 0;
    }
}


JNIEXPORT jbyteArray JNICALL
Java_com_hadir_EdccNative_nativeEncode(
        JNIEnv* env,
        jclass,
        jint encoderId,
        jbyteArray imageBytes) {

    if (!validEncoderId(encoderId) || imageBytes == nullptr) {

        throwJavaException(
                env,
                "java/lang/IllegalArgumentException",
                "Invalid encoder ID or image data."
        );

        return nullptr;
    }

    try {

        jsize imageLength =
                env->GetArrayLength(imageBytes);

        if (imageLength <= 0) {

            throwJavaException(
                    env,
                    "java/lang/IllegalArgumentException",
                    "Image data is empty."
            );

            return nullptr;
        }

        unsigned long codeSize =
                get_size_of_code_buffer_required(
                        static_cast<int>(encoderId)
                );

        if (codeSize == 0 ||
            codeSize > static_cast<unsigned long>(
                    std::numeric_limits<jsize>::max())) {

            throwNativeError(
                    env,
                    "Invalid EDCC output buffer size."
            );

            return nullptr;
        }

        std::vector<char> imageData(
                static_cast<size_t>(imageLength)
        );

        env->GetByteArrayRegion(
                imageBytes,
                0,
                imageLength,
                reinterpret_cast<jbyte*>(imageData.data())
        );

        if (env->ExceptionCheck()) {
            return nullptr;
        }

        std::vector<char> codeBuffer(
                static_cast<size_t>(codeSize),
                0
        );

        char status[STATUS_BUFFER_SIZE] = {};

        encode_palmprint_using_bytes(
                static_cast<int>(encoderId),
                imageData.data(),
                static_cast<unsigned long>(imageData.size()),
                codeBuffer.data(),
                codeSize,
                status
        );

        if (status[0] != 0) {
            throwStatusError(env, status);
            return nullptr;
        }

        jbyteArray result =
                env->NewByteArray(static_cast<jsize>(codeSize));

        if (result == nullptr) {
            return nullptr;
        }

        env->SetByteArrayRegion(
                result,
                0,
                static_cast<jsize>(codeSize),
                reinterpret_cast<const jbyte*>(codeBuffer.data())
        );

        if (env->ExceptionCheck()) {
            env->DeleteLocalRef(result);
            return nullptr;
        }

        return result;

    } catch (const std::exception& e) {

        throwNativeError(env, e.what());
        return nullptr;

    } catch (...) {

        throwNativeError(
                env,
                "Unknown error while encoding palmprint."
        );

        return nullptr;
    }
}


JNIEXPORT jdouble JNICALL
Java_com_hadir_EdccNative_nativeCompare(
        JNIEnv* env,
        jclass,
        jint encoderId,
        jbyteArray firstCode,
        jbyteArray secondCode) {

    if (!validEncoderId(encoderId) ||
        firstCode == nullptr ||
        secondCode == nullptr) {

        throwJavaException(
                env,
                "java/lang/IllegalArgumentException",
                "Invalid encoder ID or code data."
        );

        return 0.0;
    }

    try {

        jsize firstLength =
                env->GetArrayLength(firstCode);

        jsize secondLength =
                env->GetArrayLength(secondCode);

        unsigned long expectedSize =
                get_size_of_code_buffer_required(
                        static_cast<int>(encoderId)
                );

        if (firstLength <= 0 ||
            firstLength != secondLength ||
            static_cast<unsigned long>(firstLength) != expectedSize) {

            throwJavaException(
                    env,
                    "java/lang/IllegalArgumentException",
                    "Palmprint code sizes do not match."
            );

            return 0.0;
        }

        std::vector<char> firstBuffer(
                static_cast<size_t>(firstLength)
        );

        std::vector<char> secondBuffer(
                static_cast<size_t>(secondLength)
        );

        env->GetByteArrayRegion(
                firstCode,
                0,
                firstLength,
                reinterpret_cast<jbyte*>(firstBuffer.data())
        );

        if (env->ExceptionCheck()) {
            return 0.0;
        }

        env->GetByteArrayRegion(
                secondCode,
                0,
                secondLength,
                reinterpret_cast<jbyte*>(secondBuffer.data())
        );

        if (env->ExceptionCheck()) {
            return 0.0;
        }

        char status[STATUS_BUFFER_SIZE] = {};

        double score = calculate_codes_similarity(
                firstBuffer.data(),
                secondBuffer.data(),
                status
        );

        if (status[0] != 0) {
            throwStatusError(env, status);
            return 0.0;
        }

        return static_cast<jdouble>(score);

    } catch (const std::exception& e) {

        throwNativeError(env, e.what());
        return 0.0;

    } catch (...) {

        throwNativeError(
                env,
                "Unknown error while comparing palmprints."
        );

        return 0.0;
    }
}


} // extern "C"
