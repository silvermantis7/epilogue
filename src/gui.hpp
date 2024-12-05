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

#include "server.hpp"

namespace gui
{
    class Main_Frame : public wxFrame
    {
    protected:
        wxAuiNotebook* main_notebook;
        wxPanel* main_panel;

        wxTextCtrl* message_box;
        wxStatusBar* statusbar;

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

        void send_message(wxCommandEvent& event);
        wxListCtrl* message_display;

        ~Main_Frame();
    };

    class Panel : public wxPanel
    {
    protected:
        wxListCtrl* message_display;
        wxTextCtrl* message_box;
        wxBoxSizer* panel_sizer;
        std::string context;

    public:
        Panel(std::string context, wxAuiNotebook* notebook)
            : wxPanel(notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL)
        {
            panel_sizer = new wxBoxSizer(wxVERTICAL);
            this->context = context;
            this->SetSizer(panel_sizer);

            // create message display
            message_display = new wxListCtrl(this, wxID_ANY, wxDefaultPosition,
                wxDefaultSize, wxLC_ICON | wxLC_REPORT | wxLC_NO_HEADER);
            message_display->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE,
                wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
                wxT("Monospace")));
            message_display->SetForegroundColour(wxColour(0xFF, 0xFF, 0xFF));
            message_display->SetBackgroundColour(wxColour(0x18, 0x18, 0x18));
            message_display->InsertColumn(0, "user", wxLIST_FORMAT_RIGHT);
            message_display->InsertColumn(1, "message", wxLIST_FORMAT_LEFT);
            panel_sizer->Add(message_display, 1, wxEXPAND | wxALL, 5);

            // create message box
            message_box = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                wxDefaultPosition, wxSize(-1, 25), 0 | wxTE_PROCESS_ENTER);
            message_box->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE,
                wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
                wxT("Monospace")));
            message_box->SetForegroundColour(wxColour(0xFF, 0xFF, 0xFF));
            message_box->SetBackgroundColour(wxColour(0x30, 0x30, 0x30));
            panel_sizer->Add(message_box, 0, wxALL | wxEXPAND, 5);

            // create page
            this->Layout();
            panel_sizer->Fit(this);
            notebook->AddPage(this, _(context), false, wxNullBitmap);
        }

        ~Panel()
        {
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

    static void receive_messages(wxListCtrl* message_display,
        ::epilogue::Connection::pointer connection);

    static void update_statusbar(wxStatusBar* statusbar,
        std::string* channel_context);
}
