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
#include <wx/aui/auibook.h>
#include <wx/frame.h>

#include <unordered_map>

#include "server.hpp"

namespace gui
{
    class Main_Frame : public wxFrame
    {
    protected:
        wxAuiNotebook* main_notebook;
        wxPanel* main_panel;

        wxStatusBar* statusbar;
        wxBoxSizer* window_sizer;

        ::epilogue::Connection::pointer connection;
        std::thread read_thread;
        std::thread send_thread;
        std::string channel_context = "*global*";

    public:
        Main_Frame(wxWindow* parent, wxWindowID id = wxID_ANY,
            const wxString& title = _("Epilogue IRC Client"),
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxSize(800, 600),
            long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);

            ::epilogue::Connection::pointer get_connection()
            {
                return connection;
            }

        void join(std::string channel);
        wxAuiNotebook* get_notebook() { return main_notebook; }

        ~Main_Frame();
    };

    static Main_Frame* main_frame;

    class Panel : public wxPanel
    {
    protected:
        wxListCtrl* message_display;
        wxTextCtrl* message_box;
        wxBoxSizer* panel_sizer;
        std::string context;

    public:
        Panel(std::string context, wxAuiNotebook* notebook);

        void send_message(wxCommandEvent& event);

        // pointers to each message display, key is channel name/context
        static std::unordered_map<std::string, wxListCtrl*> channel_logs;

        ~Panel()
        {
            // remove self from channel_logs hashmap
            channel_logs.erase(context);

            if (context == "*global*")
            {
                // send QUIT message
                main_frame->get_connection()->send_message("QUIT\r\n");
                // close application
                Destroy();
            }
            else
            {
                // send PART message
                main_frame->get_connection()->send_message("PART " + context
                    + "\r\n");
            }
        }
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

    static void receive_messages(::epilogue::Connection::pointer connection);

    static void update_statusbar(wxStatusBar* statusbar,
        std::string* channel_context);
}
