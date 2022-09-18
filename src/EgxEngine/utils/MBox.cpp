#include "MBox.hpp"
#include <util/utilcore.hpp>

namespace Utils {

#if defined(__linux__)
    static void dummy( GtkApplication* p_app, gpointer user_data ){}
#endif

    bool EGX_API Message(const char* title, const char* text, MBox type)
    {
        static bool init = true;
        if(init) {
            init = false;
#if defined(__linux__)
    GtkApplication* p_app = gtk_application_new( "org.native_message_box.example", G_APPLICATION_FLAGS_NONE );
    g_signal_connect( p_app, "activate", G_CALLBACK(dummy), NULL );
    g_application_run( G_APPLICATION(p_app), 0, nullptr );
#endif
        }
        NMB::Icon icon;
        switch(type) {
            case MBox::ERR:
                icon = NMB::ICON_ERROR;
                break;
            case MBox::WARNING:
                icon = NMB::ICON_WARNING;
                break;
            case MBox::INFO:
                icon = NMB::ICON_INFO;
                break;
            default:
                icon = NMB::ICON_INFO;
        }
        auto result = NMB::show(title, text, icon);
        return result == NMB::Result::OK;
    }

    bool EGX_API Message(const std::string& title, const std::string& text, MBox type){
        return Utils::Message(title.c_str(), text.c_str(), type);
    }

    bool EGX_API Message(const wchar_t* title, const wchar_t* text, MBox type){
        std::wstring _wtitle = title;
        std::wstring _wtext = text;
        std::string _title = ut::ToAsciiString(_wtitle);
        std::string _text = ut::ToAsciiString(_wtext);
        return Utils::Message(_title, _text, type);
    }

    bool EGX_API Message(const std::wstring& title, const std::wstring& text, MBox type){
        std::wstring _wtitle = title.c_str();
        std::wstring _wtext = text.c_str();
        std::string _title = ut::ToAsciiString(_wtitle);
        std::string _text = ut::ToAsciiString(_wtext);
        return Utils::Message(_title, _text, type);
    }

}
