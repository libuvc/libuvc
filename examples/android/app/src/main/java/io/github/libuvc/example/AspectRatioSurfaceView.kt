package io.github.libuvc.example

import android.content.Context
import android.util.AttributeSet
import android.view.SurfaceView

/** A SurfaceView that keeps the aspect ratio of the frames it shows. */
class AspectRatioSurfaceView(context: Context, attrs: AttributeSet?) : SurfaceView(context, attrs) {
    private var aspectRatio = 0f

    fun setAspectRatio(width: Int, height: Int) {
        aspectRatio = width.toFloat() / height
        requestLayout()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        var width = MeasureSpec.getSize(widthMeasureSpec)
        var height = MeasureSpec.getSize(heightMeasureSpec)
        if (aspectRatio > 0f) {
            if (width > height * aspectRatio) {
                width = (height * aspectRatio).toInt()
            } else {
                height = (width / aspectRatio).toInt()
            }
        }
        setMeasuredDimension(width, height)
    }
}
