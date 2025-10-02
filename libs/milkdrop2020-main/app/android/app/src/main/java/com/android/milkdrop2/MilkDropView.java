/*
 * Copyright 2013 The Android Open Source Project
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

package com.android.milkdrop2;
import android.content.Context;
import android.opengl.GLSurfaceView;

import android.util.DisplayMetrics;
import android.view.MotionEvent;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

class MilkDropView extends GLSurfaceView {
    private static final String TAG = "MilkDrop";
    private static final boolean DEBUG = true;


    public MilkDropView(Context context) {
        super(context);
        MilkDropLib.setAssetManager(context.getAssets());
        // Pick an EGLConfig with RGB8 color, 16-bit depth, no stencil,
        // supporting OpenGL ES 2.0 or later backwards-compatible versions.
        setEGLConfigChooser(8, 8, 8, 8, 0, 0);
        setEGLContextClientVersion(3);
        setRenderer(new Renderer(context));
    }


    @Override public boolean onTouchEvent(MotionEvent e) {
        float x = e.getX();
        float y = e.getY();

        MilkDropLib.onTouch( ((int)e.getAction()) & MotionEvent.ACTION_MASK, x, y);
/*
        switch (e.getAction()) {
            case MotionEvent.ACTION_DOWN:
                break;
            case MotionEvent.ACTION_MOVE:
                MilkDropLib.onTouch( (int)e.getAction(), x, y);
                break;
            default:
                break;

        }*/
        return true;
    }

    private static class Renderer implements GLSurfaceView.Renderer {

        private DisplayMetrics m_displayMetrics;
        private int m_width;
        private int m_height;
        private float m_scale = 1.0f;

        public Renderer(Context context)
        {
            m_displayMetrics = context.getResources().getDisplayMetrics();
            m_scale = m_displayMetrics.density;
        }

        public void onDrawFrame(GL10 gl)
        {
            MilkDropLib.onDrawFrame(m_width, m_height, m_scale);
        }

        public void onSurfaceChanged(GL10 gl, int width, int height)
        {
            m_width = width;
            m_height = height;
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config)
        {

            MilkDropLib.init();
        }
    }
}
