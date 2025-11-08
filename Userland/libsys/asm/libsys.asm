GLOBAL sys_start_beep
GLOBAL sys_stop_beep

GLOBAL sys_fonts_text_color
GLOBAL sys_fonts_background_color
GLOBAL sys_fonts_decrease_size
GLOBAL sys_fonts_increase_size
GLOBAL sys_fonts_set_size
GLOBAL sys_clear_screen
GLOBAL sys_clear_input_buffer

GLOBAL sys_hour
GLOBAL sys_minute
GLOBAL sys_second
GLOBAL sys_sleep_milis

GLOBAL sys_circle
GLOBAL sys_rectangle
GLOBAL sys_fill_video_memory

GLOBAL sys_exec
GLOBAL sys_exit
GLOBAL sys_yield

GLOBAL sys_register_key
GLOBAL sys_register_ctrl_key

GLOBAL sys_window_width
GLOBAL sys_window_height

GLOBAL sys_get_register_snapshot

GLOBAL sys_get_character_without_display
GLOBAL sys_get_processes
GLOBAL sys_kill_process
GLOBAL sys_toggle_block_process
GLOBAL sys_get_memory_state
GLOBAL sys_set_process_priority
GLOBAL sys_create_process
GLOBAL sys_wait_process
GLOBAL sys_get_pid
GLOBAL sys_unblock_process
GLOBAL sys_alloc_memory
GLOBAL sys_free_memory
GLOBAL sys_sem_create
GLOBAL sys_sem_open
GLOBAL sys_sem_close
GLOBAL sys_sem_wait
GLOBAL sys_sem_post
GLOBAL sys_sem_get_value
GLOBAL sys_sem_reset
GLOBAL sys_sem_enter_critical_test
GLOBAL sys_sem_leave_critical_test
GLOBAL sys_sem_get_critical_count

section .text

%macro sys_int80 1
    push rbp
    mov rbp, rsp
    mov rax, %1
    int 0x80
    mov rsp, rbp
    pop rbp
    ret
%endmacro

sys_start_beep: sys_int80 0x80000000
sys_stop_beep: sys_int80 0x80000001

sys_fonts_text_color: sys_int80 0x80000002
sys_fonts_background_color: sys_int80 0x80000003



sys_fonts_decrease_size: sys_int80 0x80000007
sys_fonts_increase_size: sys_int80 0x80000008
sys_fonts_set_size: sys_int80 0x80000009
sys_clear_screen: sys_int80 0x8000000A
sys_clear_input_buffer: sys_int80 0x8000000B


sys_hour: sys_int80 0x80000010
sys_minute: sys_int80 0x80000011
sys_second: sys_int80 0x80000012

sys_circle: sys_int80 0x80000019
sys_rectangle: sys_int80 0x80000020
sys_fill_video_memory: sys_int80 0x80000021

sys_exec: sys_int80 0x800000A0
sys_exit: sys_int80 0x800000A1
sys_yield: sys_int80 0x800000A2

sys_register_key: sys_int80 0x800000B0
sys_register_ctrl_key: sys_int80 0x800000B1

sys_window_width: sys_int80 0x800000C0
sys_window_height: sys_int80 0x800000C1

sys_sleep_milis: sys_int80 0x800000D0

sys_get_register_snapshot: sys_int80 0x800000E0

sys_get_character_without_display: sys_int80 0x800000F0
sys_get_processes: sys_int80 0x800000F1
sys_kill_process: sys_int80 0x800000F2
sys_toggle_block_process: sys_int80 0x800000F3
sys_get_memory_state: sys_int80 0x800000F4
sys_set_process_priority: sys_int80 0x800000F5
sys_create_process: sys_int80 0x800000F6
sys_wait_process:        sys_int80 0x800000F7
sys_get_pid:             sys_int80 0x800000F8
sys_unblock_process:     sys_int80 0x800000F9
sys_sem_create:          sys_int80 0x800000FA
sys_sem_open:            sys_int80 0x800000FB
sys_sem_close:           sys_int80 0x800000FC
sys_sem_wait:            sys_int80 0x800000FD
sys_sem_post:            sys_int80 0x800000FE
sys_sem_get_value:       sys_int80 0x800000FF
sys_sem_enter_critical_test: sys_int80 0x80000100
sys_sem_leave_critical_test: sys_int80 0x80000101
sys_sem_get_critical_count:  sys_int80 0x80000102
sys_sem_reset:               sys_int80 0x80000103
sys_alloc_memory:            sys_int80 0x80000110
sys_free_memory:             sys_int80 0x80000111
