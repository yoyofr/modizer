/*
 * FileSelector.java - ASAP for Android
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

import java.io.File;
import java.io.IOException;

import android.app.ListActivity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ListView;

public class FileSelector extends ListActivity
{
	private ArrayAdapter<String> listAdapter;
	private File currentDir;

	private void enterDirectory(File dir)
	{
		currentDir = dir;
		listAdapter.clear();
		if (dir.getParentFile() != null)
			listAdapter.add("..");
		for (File file : dir.listFiles())
		{
			String name = file.getName();
			if (file.isDirectory())
				listAdapter.add(name + '/');
			else if (ASAP.isOurFile(name))
				listAdapter.add(name);
		}
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		listAdapter = new ArrayAdapter<String>(this, R.layout.list_item);
		setListAdapter(listAdapter);
		enterDirectory(Environment.getExternalStorageDirectory());
		setContentView(R.layout.file_list);
	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id)
	{
		String name = listAdapter.getItem(position);
		if (name.equals(".."))
			enterDirectory(currentDir.getParentFile());
		else if (name.endsWith("/"))
			enterDirectory(new File(currentDir, name));
		else {
			Uri uri = Uri.fromFile(new File(currentDir, name));
			Intent intent = new Intent(Intent.ACTION_VIEW, uri, this, Player.class);
			startActivity(intent);
		}
	}
}
