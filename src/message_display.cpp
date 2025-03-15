#include "message_display.hpp"

gui::Message_Display::Message_Display(wxWindow* parent)
: wxGLCanvas(parent, disp_attrs(), wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
    // create OpenGL context
    gl_context = new wxGLContext(this);
    assert(gl_context->IsOK());
    CallAfter([&]() { gl_context->SetCurrent(*this); });
}

wxGLAttributes& gui::Message_Display::disp_attrs()
{
    disp_attrs_.PlatformDefaults().MinRGBA(8, 8, 8, 8).DoubleBuffer().Depth(24)
        .EndList();
    return disp_attrs_;
}
