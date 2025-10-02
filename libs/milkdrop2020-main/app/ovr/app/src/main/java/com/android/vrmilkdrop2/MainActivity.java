/************************************************************************************

Filename    :   MainActivity.java
Content     :   
Created     :   
Authors     :   

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/
package com.android.vrmilkdrop2;

/*
When using NativeActivity, we currently need to handle loading of dependent 
shared libraries manually before a shared library that depends on them
is loaded, since there is not currently a way to specify a shared library
dependency for NativeActivity via the manifest meta-data.

The simplest method for doing so is to subclass NativeActivity with
an empty activity that calls System.loadLibrary on the dependent
libraries, which is unfortunate when the goal is to write a pure
native C/C++ only Android activity.

A native-code only solution is to load the dependent libraries
dynamically using dlopen(). However, there are a few considerations,
see: https://groups.google.com/forum/#!msg/android-ndk/l2E2qh17Q6I/wj6s_6HSjaYJ
1. Only call dlopen() if you're sure it will succeed as the bionic
dynamic linker will remember if dlopen failed and will not re-try
a dlopen on the same lib a second time.
2. Must rememeber what libraries have already been loaded to avoid
infinitely looping when libraries have circular dependencies.
*/

import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;

import android.Manifest;
import android.content.pm.PackageManager;
import android.support.v4.content.ContextCompat;
import android.support.v4.content.PermissionChecker;
import android.support.v4.app.ActivityCompat;

import android.os.Environment;
import java.io.File;

public class MainActivity extends android.app.NativeActivity implements
		ActivityCompat.OnRequestPermissionsResultCallback {
	
	/** Load jni .so on initialization */
	static {
		System.loadLibrary( "mdp" );
	}

	public static final String TAG = "VrMilkDrop";

	public static native void nativeInitWithPermissions( int recordAudioPermissionGranted, int externalStoragePermissionGranted );



	boolean externalStoragePermissionGranted = false;
	boolean recordAudioPermissionGranted = false;


	protected void setNativePermissions()
	{
		MainActivity.nativeInitWithPermissions( recordAudioPermissionGranted ? 1 : 0, externalStoragePermissionGranted ? 1 : 0);
	}



	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.d(TAG, "onCreate");

		super.onCreate(savedInstanceState);

		if (ContextCompat.checkSelfPermission(this,
				Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED) {
			recordAudioPermissionGranted = true;
		}


		if (ContextCompat.checkSelfPermission(this,
				Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
			externalStoragePermissionGranted = true;
		}

		if (!recordAudioPermissionGranted || !externalStoragePermissionGranted)
		{
			requestPermissions();
		}
		else
		{
			Log.d(TAG, "onCreate - permissions already granted.");
			setNativePermissions();

		}

	}

	protected void requestPermissions()
	{
		ActivityCompat.requestPermissions(
				this,
				new String[]{
						Manifest.permission.RECORD_AUDIO,
						Manifest.permission.READ_EXTERNAL_STORAGE,
				},
				0);
	}

	// ActivityCompat.OnRequestPermissionsResultCallback
	@Override
	public void onRequestPermissionsResult( int requestCode, String[] permissions, int[] grantResults ) {
		for (int i = 0; i < permissions.length; i++) 
		{
			switch (permissions[i]) 
			{
				case Manifest.permission.RECORD_AUDIO: {
					if (grantResults[i] == PackageManager.PERMISSION_GRANTED)
					{
						Log.d( TAG, "onRequestPermissionsResult permission GRANTED" );
						recordAudioPermissionGranted = true;
					}
					else
					{
						Log.d( TAG, "onRequestPermissionsResult permission DENIED" );
						recordAudioPermissionGranted = false;
					}
					break;
				}

				case Manifest.permission.READ_EXTERNAL_STORAGE:
				{
					if (grantResults[i] == PackageManager.PERMISSION_GRANTED) 
					{
						Log.d( TAG, "onRequestPermissionsResult permission GRANTED" );
						externalStoragePermissionGranted = true;
					}
					else 
					{
						Log.d( TAG, "onRequestPermissionsResult permission DENIED" );
						externalStoragePermissionGranted = false;
					}
					break;

				}

				default:
					break;
			}
		}

		runOnUiThread( new Thread() {
			@Override
			public void run() {
				setNativePermissions();
			}
		});

	}  

}
