/*
 * Player.java - ASAP for Android
 *
 * Copyright (C) 2010  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

package net.sf.asap;

import android.app.AlertDialog;
import android.app.Activity;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.io.IOException;
import org.apache.http.HttpResponse;
import org.apache.http.StatusLine;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;

public class Player extends Activity implements Runnable, OnClickListener
{
	private final ASAP asap = new ASAP();
	private boolean stop;

	private void showError(int messageId)
	{
		new AlertDialog.Builder(this).setMessage(messageId).show();
		//FIXME finish();
	}

	private void setTag(int controlId, String value)
	{
		TextView control = (TextView) findViewById(controlId);
		if (value.length() == 0)
			control.setVisibility(View.GONE);
		else {
			control.setText(value);
			control.setVisibility(View.VISIBLE);
		}
	}

	public void run()
	{
		ASAP_ModuleInfo module_info = asap.getModuleInfo();
		int song = module_info.default_song;
		int duration = module_info.loops[song] ? -1 : module_info.durations[song];
		asap.playSong(song, duration);

		int config = module_info.channels == 1 ? AudioFormat.CHANNEL_CONFIGURATION_MONO : AudioFormat.CHANNEL_CONFIGURATION_STEREO;
		int len = AudioTrack.getMinBufferSize(ASAP.SAMPLE_RATE, config, AudioFormat.ENCODING_PCM_8BIT);
		if (len < 16384)
			len = 16384;
		final byte[] buffer = new byte[len];
		AudioTrack audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, ASAP.SAMPLE_RATE,
			config, AudioFormat.ENCODING_PCM_8BIT, len, AudioTrack.MODE_STREAM);
		audioTrack.play();
		do {
			synchronized (this) {
				if (this.stop) {
					audioTrack.stop();
					return;
				}
			}
			len = asap.generate(buffer, ASAP.FORMAT_U8);
			audioTrack.write(buffer, 0, len);
		} while (len == buffer.length);
		audioTrack.flush();
		audioTrack.stop();
	}

	public void onClick(View v)
	{
		finish();
	}

	private static InputStream httpGet(Uri uri)
		throws IOException
	{
		DefaultHttpClient client = new DefaultHttpClient();
		HttpGet request = new HttpGet(uri.toString());
		HttpResponse response = client.execute(request);
		StatusLine status = response.getStatusLine();
		if (status.getStatusCode() != 200)
			throw new IOException("HTTP error " + status);
		return response.getEntity().getContent();
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		Uri uri = getIntent().getData();
		final byte[] module = new byte[ASAP.MODULE_MAX];
		int module_len;
		try {
			InputStream is;
			try {
				is = getContentResolver().openInputStream(uri);
			}
			catch (FileNotFoundException ex) {
				if (uri.getScheme().equals("http"))
					is = httpGet(uri);
				else
					throw ex;
			}
			module_len = ASAP.readAndClose(is, module);
		}
		catch (IOException ex) {
			showError(R.string.error_reading_file);
			return;
		}
		try {
			asap.load(uri.getLastPathSegment(), module, module_len);
		}
		catch (IllegalArgumentException ex)
		{
			showError(R.string.invalid_file);
			return;
		}
		ASAP_ModuleInfo module_info = asap.getModuleInfo();

		setContentView(R.layout.playing);
		setTag(R.id.name, module_info.name);
		setTag(R.id.author, module_info.author);
		setTag(R.id.date, module_info.date);

		findViewById(R.id.stop_button).setOnClickListener(this);
		this.stop = false;
		new Thread(this).start();
	}

	@Override
	public void onStop()
	{
		super.onStop();
		synchronized (this) {
			this.stop = true;
		}
	}
}
