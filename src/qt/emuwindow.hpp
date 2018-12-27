#ifndef EMUWINDOW_HPP
#define EMUWINDOW_HPP

#include <chrono>
#include <QMainWindow>
#include <QPaintEvent>

#include "debugwindow.hpp"
#include "emuthread.hpp"

#include "../qt/settings.hpp"
#include "../core/emulator.hpp"

class EmuWindow : public QMainWindow
{
    Q_OBJECT
    private:
        DebugWindow debug;
        EmuThread emuthread;
        std::string title;
        std::string ROM_path;
        QImage final_image;
        std::chrono::system_clock::time_point old_update_time;
        double framerate_avg;

        QMenu* file_menu;
        QMenu* options_menu;
        QMenu* debug_menu;
        QAction* load_rom_action;
        QAction* load_bios_action;
        QAction* load_state_action;
        QAction* save_state_action;
        QAction* exit_action;
        QAction* show_debugger_action;
        int scale_factor;
    public:
        explicit EmuWindow(QWidget *parent = nullptr);
        int init(int argc, char** argv);
        int load_exec(const char* file_name, bool skip_BIOS);
        int run_gsdump(const char* file_name);

        void create_menu();

        void paintEvent(QPaintEvent *event) override;
        void closeEvent(QCloseEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;
        void keyReleaseEvent(QKeyEvent *event) override;
    protected:
    #ifndef QT_NO_CONTEXTMENU
        void contextMenuEvent(QContextMenuEvent* event) override;
    #endif
    
    signals:
        void shutdown();
        void press_key(PAD_BUTTON button);
        void release_key(PAD_BUTTON button);
    public slots:
        void update_FPS(int FPS);
        void draw_frame(uint32_t* buffer, int inner_w, int inner_h, int final_w, int final_h);
        void open_file_no_skip();
        void open_file_skip();
        void load_state();
        void save_state();
        void show_debugger();
        void emu_error(QString err);
        void emu_nonfatal_error(QString err);
        void emu_breakpoint(unsigned int addr);
        void emu_toggle_debug_break(bool running);
};

#endif // EMUWINDOW_HPP
