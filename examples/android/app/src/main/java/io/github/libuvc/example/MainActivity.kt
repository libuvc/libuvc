package io.github.libuvc.example

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.hardware.usb.UsbConstants
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.view.Surface
import android.view.SurfaceHolder
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Spinner
import android.widget.TextView
import java.util.concurrent.Executors

/**
 * Shows the picture of a UVC camera.
 *
 * All the USB plumbing is here, with the platform APIs: finding the camera,
 * getting permission to use it, and noticing when it is unplugged. libuvc
 * takes over once the device is open, in [UvcCamera].
 */
class MainActivity : Activity(), SurfaceHolder.Callback, AdapterView.OnItemSelectedListener {
    private lateinit var usbManager: UsbManager
    private lateinit var preview: AspectRatioSurfaceView
    private lateinit var modeSpinner: Spinner
    private lateinit var statusView: TextView

    // The UvcCamera calls that block run here, one at a time.
    private val cameraExecutor = Executors.newSingleThreadExecutor()
    private val handler = Handler(Looper.getMainLooper())

    // Everything below belongs to the main thread.
    private var started = false
    private var device: UsbDevice? = null // being opened, or open
    private var camera: UvcCamera? = null
    private var surface: Surface? = null
    private var streaming: String? = null
    private var lastFrames = 0
    private var lastTime = 0L

    private val usbReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val dev = intent.usbDevice ?: return
            when (intent.action) {
                ACTION_USB_PERMISSION -> if (dev == device) {
                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        openCamera(dev)
                    } else {
                        device = null
                        setStatus(getString(R.string.usb_permission_denied))
                    }
                }
                UsbManager.ACTION_USB_DEVICE_ATTACHED -> findCamera()
                UsbManager.ACTION_USB_DEVICE_DETACHED -> if (dev == device) {
                    closeCamera()
                    // Another camera, or "Connect a UVC camera."
                    findCamera()
                }
            }
        }
    }

    private val statsUpdater = object : Runnable {
        override fun run() {
            updateStats()
            handler.postDelayed(this, 1000)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        usbManager = getSystemService(UsbManager::class.java)
        preview = findViewById(R.id.preview)
        modeSpinner = findViewById(R.id.mode)
        statusView = findViewById(R.id.status)
        preview.holder.addCallback(this)
        modeSpinner.onItemSelectedListener = this
        registerUsbReceiver()
    }

    @SuppressLint("UnspecifiedRegisterReceiverFlag")
    private fun registerUsbReceiver() {
        val filter = IntentFilter().apply {
            addAction(ACTION_USB_PERMISSION)
            addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
            addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        }
        // The permission result comes from our own PendingIntent, and the
        // other two from the system: none of them needs an exported receiver.
        if (Build.VERSION.SDK_INT >= 33) {
            registerReceiver(usbReceiver, filter, RECEIVER_NOT_EXPORTED)
        } else {
            // Receivers registered at runtime are always exported before
            // Android 13. A forged permission result gets no further than
            // UsbManager.openDevice(), which checks the permission itself.
            registerReceiver(usbReceiver, filter)
        }
    }

    override fun onStart() {
        super.onStart()
        started = true
        // Since Android 9, only apps that hold the CAMERA permission can get
        // USB permission for a device with a video interface: without it,
        // UsbManager.requestPermission() fails without asking the user.
        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            findCamera()
        } else {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA)
        }
        handler.post(statsUpdater)
    }

    override fun onStop() {
        started = false
        handler.removeCallbacks(statsUpdater)
        // Release the camera while in the background, for other apps.
        closeCamera()
        super.onStop()
    }

    override fun onDestroy() {
        unregisterReceiver(usbReceiver)
        // Lets the close() queued by onStop() run.
        cameraExecutor.shutdown()
        super.onDestroy()
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        // A camera was plugged in while showing (USB_DEVICE_ATTACHED).
        setIntent(intent)
        findCamera()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        if (requestCode != REQUEST_CAMERA) return
        if (grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
            findCamera()
        } else {
            setStatus(getString(R.string.camera_permission_denied))
        }
    }

    private fun findCamera() {
        if (!started || device != null) return
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) return

        // Prefer the camera that started this activity, if any.
        val devices = usbManager.deviceList
        val dev = intent.usbDevice?.takeIf { it.isVideo() && devices.containsKey(it.deviceName) }
            ?: devices.values.firstOrNull { it.isVideo() }
        if (dev == null) {
            setStatus(getString(R.string.no_camera))
            return
        }

        device = dev
        if (usbManager.hasPermission(dev)) {
            openCamera(dev)
            return
        }
        setStatus(getString(R.string.waiting_for_permission, dev.displayName))
        // The system adds EXTRA_DEVICE and EXTRA_PERMISSION_GRANTED to this
        // intent, so the PendingIntent must be mutable, and Android 14
        // requires a mutable PendingIntent to be explicit.
        val result = Intent(ACTION_USB_PERMISSION).setPackage(packageName)
        val flags = if (Build.VERSION.SDK_INT >= 31) PendingIntent.FLAG_MUTABLE else 0
        usbManager.requestPermission(dev, PendingIntent.getBroadcast(this, 0, result, flags))
    }

    private fun openCamera(dev: UsbDevice) {
        setStatus(getString(R.string.opening, dev.displayName))
        cameraExecutor.execute {
            val result = runCatching { UvcCamera.open(usbManager, dev) }
            handler.post {
                result.fold(
                    onSuccess = { cam -> onCameraOpened(dev, cam) },
                    onFailure = { e ->
                        if (dev == device) {
                            device = null
                            setStatus(getString(R.string.open_failed, e.message))
                        }
                    },
                )
            }
        }
    }

    private fun onCameraOpened(dev: UsbDevice, cam: UvcCamera) {
        if (dev != device) {
            // Stopped, or unplugged, in the meantime.
            cameraExecutor.execute { cam.close() }
            return
        }
        camera = cam
        cam.setSurface(surface)
        if (cam.modes.isEmpty()) {
            setStatus(getString(R.string.no_modes))
            return
        }
        // Selecting a mode starts it, in onItemSelected().
        modeSpinner.adapter =
            ArrayAdapter(this, android.R.layout.simple_spinner_dropdown_item, cam.modes)
        modeSpinner.setSelection(cam.defaultMode)
        modeSpinner.visibility = View.VISIBLE
    }

    private fun closeCamera() {
        val cam = camera
        camera = null
        device = null
        streaming = null
        modeSpinner.visibility = View.GONE
        if (cam != null) {
            // Stop drawing now; closing can take a while.
            cam.setSurface(null)
            cameraExecutor.execute { cam.close() }
        }
    }

    override fun onItemSelected(parent: AdapterView<*>, view: View?, position: Int, id: Long) {
        val cam = camera ?: return
        val mode = cam.modes[position]
        streaming = null
        setStatus(getString(R.string.starting, mode))
        cameraExecutor.execute {
            val result = runCatching { cam.start(position) }
            handler.post {
                if (cam !== camera) return@post
                result.fold(
                    onSuccess = { size ->
                        preview.setAspectRatio(size.width, size.height)
                        streaming = mode
                        lastTime = 0
                    },
                    onFailure = { e -> setStatus(getString(R.string.start_failed, mode, e.message)) },
                )
            }
        }
    }

    override fun onNothingSelected(parent: AdapterView<*>) {}

    override fun surfaceCreated(holder: SurfaceHolder) {
        surface = holder.surface
        camera?.setSurface(surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {}

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        // Nothing may draw on the surface once this returns: setSurface()
        // waits for a frame being drawn to be done.
        surface = null
        camera?.setSurface(null)
    }

    private fun updateStats() {
        val cam = camera ?: return
        val mode = streaming ?: return
        val frames = cam.frameCount()
        val now = SystemClock.elapsedRealtime()
        if (lastTime != 0L) {
            val fps = (frames - lastFrames) * 1000f / (now - lastTime)
            setStatus(getString(R.string.streaming, mode, fps))
        }
        lastFrames = frames
        lastTime = now
    }

    private fun setStatus(text: String) {
        statusView.text = text
    }
}

private const val ACTION_USB_PERMISSION = "io.github.libuvc.example.USB_PERMISSION"
private const val REQUEST_CAMERA = 1

private val Intent.usbDevice: UsbDevice?
    // The typed getParcelableExtra() is unreliable before Android 14.
    get() = if (Build.VERSION.SDK_INT >= 34) {
        getParcelableExtra(UsbManager.EXTRA_DEVICE, UsbDevice::class.java)
    } else {
        @Suppress("DEPRECATION")
        getParcelableExtra(UsbManager.EXTRA_DEVICE)
    }

/** Whether this has a video interface: most webcams are composite devices, so
 * the device class alone does not tell. */
private fun UsbDevice.isVideo() =
    (0 until interfaceCount).any { getInterface(it).interfaceClass == UsbConstants.USB_CLASS_VIDEO }

private val UsbDevice.displayName: String
    get() = productName ?: deviceName
