#ifndef EMUTHREAD_HPP
#define EMUTHREAD_HPP

#include <chrono>

#include <QMutex>
#include <QThread>
#include <string>

#include "../core/emulator.hpp"
#include "../core/errors.hpp"
#include "../core/ee/emotion_breakpoint.hpp"

#define GSDUMP_BUFFERED_MESSAGES 100000

enum PAUSE_EVENT
{
    GAME_NOT_LOADED,
    FILE_DIALOG,
    MESSAGE_BOX,
    FRAME_ADVANCE,
    DEBUGGER,
    USER_REQUESTED
};

class EmuThread : public QThread
{
    Q_OBJECT
    private:
        bool abort;
        uint32_t pause_status;
        QMutex emu_mutex, load_mutex, pause_mutex;
        Emulator e;

        std::chrono::system_clock::time_point old_frametime;
        std::ifstream gsdump;
        std::atomic_bool gsdump_reading;
        GSMessage* gsdump_read_buffer;
        int buffered_gs_messages;
        int current_gs_message;

        void gsdump_run();
    public:
        EmuThread();
        ~EmuThread();

        void reset();

        void set_skip_BIOS_hack(SKIP_HACK skip);
        void set_vu1_mode(VU_MODE mode);
        void load_BIOS(const uint8_t* BIOS);
        void load_ELF(const uint8_t* ELF, uint64_t ELF_size);
        void load_CDVD(const char* name, CDVD_CONTAINER type);
        
        EEBreakpointList* get_ee_breakpoint_list();
        IOPBreakpointList* get_iop_breakpoint_list();
        bool is_paused();

        bool load_state(const char* name);
        bool save_state(const char* name);
        bool gsdump_read(const char* name);
        void gsdump_write_toggle();
        void gsdump_single_frame();
        GSMessage& get_next_gsdump_message();
        bool gsdump_eof();
        bool frame_advance;
    protected:
        void run() override;
    signals:
        void completed_frame(uint32_t* buffer, int inner_w, int inner_h, int final_w, int final_h);
        void update_FPS(int FPS);
        void emu_error(QString err);
        void emu_non_fatal_error(QString err);
    public slots:
        void shutdown();
        void press_key(PAD_BUTTON button);
        void release_key(PAD_BUTTON button);
        void update_joystick(JOYSTICK joystick, JOYSTICK_AXIS axis, uint8_t val);
        void pause(PAUSE_EVENT event);
        void unpause(PAUSE_EVENT event);
};

#endif // EMUTHREAD_HPP
