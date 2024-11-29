#include <wx/wx.h>

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>

#include <wx/listctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/frame.h>

#include "server.hpp"

namespace gui
{
    class Frame : public wxFrame
    {
    protected:
        wxTextCtrl* message_box;
        wxStatusBar* statusbar;

        ::epilogue::Connection::pointer connection;
        std::thread read_thread;
        std::thread send_thread;
        std::string channel_context = "#test";

    public:
        Frame(wxWindow* parent, wxWindowID id = wxID_ANY,
            const wxString& title = _("Epilogue IRC Client"),
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxSize(800, 600),
            long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);

        void send_message(wxCommandEvent& event);
        wxListCtrl* message_display;

        ~Frame();
    };
    
    class Connect_Dialog : public wxDialog
    {
    protected:
        wxTextCtrl* server_input;
        wxButton* connect_button;
        std::string host_, port_;

    public:    
        Connect_Dialog(wxWindow* parent, wxWindowID id = wxID_ANY,
            const wxString& title = _("connect to server"),
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxSize(-1, -1),
            long style = wxDEFAULT_DIALOG_STYLE);
        
        void connect(wxCommandEvent& event);
        std::string host() { return host_; }
        std::string port() { return port_; }

        ~Connect_Dialog();
    };

    class Epilogue : public wxApp
    {
    public:
        bool OnInit() override;
    };

    static void receive_messages(wxListCtrl* message_display,
        ::epilogue::Connection::pointer connection);

    static void update_statusbar(wxStatusBar* statusbar,
        std::string* channel_context);
}
