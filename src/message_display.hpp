#pragma once

#include <wx/glcanvas.h>

namespace gui
{
    class Message_Display : public wxGLCanvas
    {
        wxGLAttributes disp_attrs_;
        wxGLAttributes& disp_attrs();
        wxGLContext* gl_context;

    public:
        Message_Display(wxWindow* parent);
    };
}
