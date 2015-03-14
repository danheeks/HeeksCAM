// ToolImage.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "ToolImage.h"

float ToolImage::m_button_scale = 0.25;
const int ToolImage::full_size = 96;
const int ToolImage::default_bitmap_size = 24;

ToolImage::ToolImage(const wxString& name, bool full_path_given):wxImage(full_path_given?name:(wxGetApp().GetResFolder() + _T("/bitmaps/") + name + _T(".png")), wxBITMAP_TYPE_PNG)
{
	int width = GetWidth();
	int height = GetHeight();
	float button_scale = m_button_scale;
	int new_width = (int)(button_scale * width);
	int new_height = (int)(button_scale * height);
	Rescale(new_width, new_height);
}

int ToolImage::GetBitmapSize()
{
	return (int)(m_button_scale * full_size);
}

void ToolImage::SetBitmapSize(int size)
{
	m_button_scale = (float)size / (float)full_size;
}
