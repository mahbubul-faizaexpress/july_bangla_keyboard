package org.julybangla.keyboard

/** JNI bridge to the shared C++ engine (`src/engine`, built as libjulyengine.so). */
internal object NativeEngine {
    init {
        System.loadLibrary("julyengine")
    }

    @JvmStatic external fun create(classic: Boolean): Long
    @JvmStatic external fun destroy(handle: Long)
    @JvmStatic external fun pressKey(handle: Long, scan: Int, shift: Boolean): String
    @JvmStatic external fun backspace(handle: Long): String
    @JvmStatic external fun reset(handle: Long)
    @JvmStatic external fun keyLabel(scan: Int, shift: Boolean): String
}

/**
 * What the keyboard must do after a key, exactly as on Windows: replace the composing text
 * with [commit] + [composition], finalize [commit], keep [composition] as the new
 * composing text. If [eaten] is false the key's own character is also inserted.
 */
internal data class EditResult(val eaten: Boolean, val commit: String, val composition: String) {
    companion object {
        /** Decodes the bridge's packed form: '1' or '0', commit, U+0000, composition. */
        fun decode(packed: String): EditResult {
            val separator = packed.indexOf('\u0000', startIndex = 1)
            require(packed.isNotEmpty() && separator > 0) { "malformed engine result" }
            return EditResult(
                eaten = packed[0] == '1',
                commit = packed.substring(1, separator),
                composition = packed.substring(separator + 1),
            )
        }
    }
}

/** One Bijoy composer (Unicode or Classic output). Not thread-safe; used on the IME thread. */
internal class Composer(classic: Boolean) : AutoCloseable {
    private var handle = NativeEngine.create(classic).also { check(it != 0L) { "out of memory" } }

    fun pressKey(scan: Int, shift: Boolean) = EditResult.decode(NativeEngine.pressKey(handle, scan, shift))
    fun backspace() = EditResult.decode(NativeEngine.backspace(handle))
    fun reset() = NativeEngine.reset(handle)

    override fun close() {
        if (handle != 0L) {
            NativeEngine.destroy(handle)
            handle = 0L
        }
    }
}
