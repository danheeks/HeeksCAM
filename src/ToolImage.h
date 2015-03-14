// ToolImage.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#pragma once

#include <wx/image.h>

class ToolImage: public wxImage{
public:
	static float m_button_scale;
	static const int full_size;
	static const int default_bitmap_size;

	ToolImage(const wxString& name, bool full_path_given = false);

	static int GetBitmapSize();
	static void SetBitmapSize(int size);
};

