package io.github.libuvc.example

import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbManager
import android.util.Size
import android.view.Surface
import java.io.IOException

/**
 * A UVC camera, driven by libuvc.
 *
 * [open], [start], [stop] and [close] wait on USB transfers: call them from a
 * background thread, always the same one. [setSurface] and [frameCount] are
 * quick, and safe to call from any thread.
 */
class UvcCamera private constructor(
    // libuvc works on this connection's file descriptor, which therefore has
    // to stay open until the camera is closed.
    private val connection: UsbDeviceConnection,
    private var handle: Long,
) : AutoCloseable {

    /** What the camera can stream and this example can show, as labels. */
    val modes: List<String> = nativeModes(handle).asList()

    /** The camera's own default mode, as an index into [modes]. */
    val defaultMode: Int = nativeDefaultMode(handle)

    private val lock = Any()

    /** Streams [mode], replacing any previous stream, and returns its frame size. */
    @Throws(IOException::class)
    fun start(mode: Int): Size {
        val size = nativeStart(checkOpen(), mode)
        return Size(size[0], size[1])
    }

    fun stop() = nativeStop(checkOpen())

    /** Frames are drawn on [surface], or dropped while there is none. */
    fun setSurface(surface: Surface?) = synchronized(lock) {
        if (handle != 0L) nativeSetSurface(handle, surface)
    }

    /** Number of frames received so far. */
    fun frameCount(): Int = synchronized(lock) {
        if (handle != 0L) nativeFrameCount(handle) else 0
    }

    override fun close() {
        // Clear the handle under the lock, so that setSurface() and
        // frameCount() cannot use it while nativeClose() frees it.
        val h = synchronized(lock) { handle.also { handle = 0 } }
        if (h != 0L) nativeClose(h)
        // Only now: libuvc does not close the file descriptor itself.
        connection.close()
    }

    private fun checkOpen(): Long = handle.also { check(it != 0L) { "Camera closed" } }

    companion object {
        init {
            System.loadLibrary("uvcexample")
        }

        /** Opens [device], which the app must have USB permission for. */
        @Throws(IOException::class)
        fun open(usbManager: UsbManager, device: UsbDevice): UvcCamera {
            val connection = usbManager.openDevice(device)
                ?: throw IOException("Cannot open ${device.deviceName}")
            val handle = try {
                nativeOpen(connection.fileDescriptor)
            } catch (e: Throwable) {
                connection.close()
                throw e
            }
            return UvcCamera(connection, handle)
        }

        // Registered by JNI_OnLoad(), in uvc_jni.c.
        @JvmStatic private external fun nativeOpen(fd: Int): Long
        @JvmStatic private external fun nativeModes(handle: Long): Array<String>
        @JvmStatic private external fun nativeDefaultMode(handle: Long): Int
        @JvmStatic private external fun nativeStart(handle: Long, mode: Int): IntArray
        @JvmStatic private external fun nativeStop(handle: Long)
        @JvmStatic private external fun nativeSetSurface(handle: Long, surface: Surface?)
        @JvmStatic private external fun nativeFrameCount(handle: Long): Int
        @JvmStatic private external fun nativeClose(handle: Long)
    }
}
