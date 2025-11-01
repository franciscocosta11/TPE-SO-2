#include <syscallDispatcher.h>
#include <stddef.h>
#include <sound.h>
#include <keyboard.h>
#include <fonts.h>
#include <lib.h>
#include <video.h>
#include <time.h>
#include <process.h>
#include <MemoryManager.h>
#include <string.h>
#include <interrupts.h>

extern int64_t register_snapshot[18];
extern int64_t register_snapshot_taken;

// @todo Note: Technically.. registers on the stack are modifiable (since its a struct pointer, not struct). 
int32_t syscallDispatcher(Registers * registers) {
	switch(registers->rax){
		case 3: return sys_read(registers->rdi, (signed char *) registers->rsi, registers->rdx);
		// Note: Register parameters are 64-bit
		case 4: return sys_write(registers->rdi, (char *) registers->rsi, registers->rdx);
		
		case 0x80000000: return sys_start_beep(registers->rdi);
		case 0x80000001: return sys_stop_beep();
		case 0x80000002: return sys_fonts_text_color(registers->rdi);
		case 0x80000003: return sys_fonts_background_color(registers->rdi);
		case 0x80000004: /* Reserved for sys_set_italics */
		case 0x80000005: /* Reserved for sys_set_bold */
		case 0x80000006: /* Reserved for sys_set_underline */
			return -1;
		case 0x80000007: return sys_fonts_decrease_size();
		case 0x80000008: return sys_fonts_increase_size();
		case 0x80000009: return sys_fonts_set_size((uint8_t) registers->rdi);
		case 0x8000000A: return sys_clear_screen();
		case 0x8000000B: return sys_clear_input_buffer();

		case 0x80000010: return sys_hour((int *) registers->rdi);
		case 0x80000011: return sys_minute((int *) registers->rdi);
		case 0x80000012: return sys_second((int *) registers->rdi);

		case 0x80000019: return sys_circle(registers->rdi, registers->rsi, registers->rdx, registers->rcx);
		case 0x80000020: return sys_rectangle(registers->rdi, registers->rsi, registers->rdx, registers->rcx, registers->r8);
		case 0x80000021: return sys_fill_video_memory(registers->rdi);

		case 0x800000A0: return sys_exec((int (*)(void)) registers->rdi);
        case 0x800000A1: return sys_exit((int32_t)registers->rdi);

		case 0x800000B0: return sys_register_key((uint8_t) registers->rdi, (SpecialKeyHandler) registers->rsi);
		case 0x800000B1: return sys_register_ctrl_key((uint8_t) registers->rdi, (SpecialKeyHandler) registers->rsi);

		case 0x800000C0: return sys_window_width();
		case 0x800000C1: return sys_window_height();

		case 0x800000D0: return sys_sleep_milis(registers->rdi);

		case 0x800000E0: return sys_get_register_snapshot((int64_t *) registers->rdi);

		case 0x800000F0: return sys_get_character_without_display();
		case 0x800000F1: return sys_get_processes((ProcessInfo *) registers->rdi, registers->rsi);
		case 0x800000F2: return sys_kill_process((int32_t)registers->rdi);
		case 0x800000F3: return sys_toggle_block_process((int32_t)registers->rdi);
		case 0x800000F4: return sys_get_memory_state((char *)registers->rdi, registers->rsi);
		case 0x800000F5: return sys_set_process_priority((int32_t)registers->rdi, (int32_t)registers->rsi);
		case 0x800000F6: return sys_create_process((char *)registers->rdi, (void (*)(void *)) registers->rsi, (char **) registers->rdx, (uint32_t) registers->rcx, (void *) registers->r8, (uint64_t) registers->r9, (int) registers->r10 ,(uint8_t) registers->r11);
        case 0x800000F7: return sys_wait_process((int32_t)registers->rdi);
		default: return 0;
	}
}

// ==================================================================
// Linux syscalls
// ==================================================================

int32_t sys_write(int32_t fd, char * __user_buf, int32_t count) {
    return printToFd(fd, __user_buf, count);
}

int32_t sys_read(int32_t fd, signed char * __user_buf, int32_t count) {
	int32_t i;
	int8_t c;
	for(i = 0; i < count && (c = getKeyboardCharacter(AWAIT_RETURN_KEY | SHOW_BUFFER_WHILE_TYPING)) != EOF; i++){
		*(__user_buf + i) = c;
	}
    return i;
}

// ==================================================================
// Custom system calls
// ==================================================================

int32_t sys_start_beep(uint32_t nFrequence) {
	play_sound(nFrequence);
	return 0;
}

int32_t sys_stop_beep(void) {
	setSpeaker(SPEAKER_OFF);
	return 0;
}

int32_t sys_fonts_text_color(uint32_t color) {
	setTextColor(color);
	return 0;
}

int32_t sys_fonts_background_color(uint32_t color) {
	setBackgroundColor(color);
	return 0;
}

int32_t sys_fonts_decrease_size(void) {
	return decreaseFontSize();
}

int32_t sys_fonts_increase_size(void) {
	return increaseFontSize();
}

int32_t sys_fonts_set_size(uint8_t size) {
	return setFontSize(size);
}

int32_t sys_clear_screen(void) {
	clear();
	return 0;
}

int32_t sys_clear_input_buffer(void) {
	while(clearBuffer() != 0);
	return 0;
}

uint16_t sys_window_width(void) {
	return getWindowWidth();
}

uint16_t sys_window_height(void) {
	return getWindowHeight();
}

int32_t sys_get_processes(ProcessInfo *userBuffer, uint64_t capacity) {
	if (userBuffer == NULL || capacity == 0) {
		return 0;
	}

	size_t maxEntries = (size_t)capacity;
	if (maxEntries > MAX_PROCESSES) {
		maxEntries = MAX_PROCESSES;
	}

	size_t written = getProcessSnapshot(userBuffer, maxEntries);

	return (int32_t)written;
}

int32_t sys_kill_process(int32_t pid) {
	return killProcess(pid);
}

int32_t sys_toggle_block_process(int32_t pid) {
	return toggleProcessBlock(pid);
}

int32_t sys_get_memory_state(char *userBuffer, uint64_t capacity) {
	if (userBuffer == NULL || capacity == 0) {
		return 0;
	}

	const char *status = consultMemory();
	if (status == NULL) {
		return 0;
	}

	size_t len = 0;
	while (status[len] != '\0' && len + 1 < capacity) {
		userBuffer[len] = status[len];
		len++;
	}

	if (capacity > 0) {
		userBuffer[len] = '\0';
	}

	return (int32_t)len;
}

int32_t sys_set_process_priority(int32_t pid, int32_t priority) {
	return setProcessPriority(pid, priority);
}

int32_t sys_create_process(char* name, void (*entry)(void *), char **argv, uint32_t argc, void *stackBase, uint64_t stackSize, int priority, uint8_t isForeground) {
	return createProcess(name, entry, argv, argc, stackBase, stackSize, priority, isForeground)->pid;
}

int32_t sys_wait_process(int32_t pid) {
	waitProcess(pid);
	return 0;
}

int32_t sys_exit(int32_t status) {
	exitCurrentProcess(status);
	// Conmutar inmediatamente para no seguir ejecutando el proceso terminado
	contextSwitch();
	return 0; // No debería alcanzarse
}

// ==================================================================
// Date system calls
// ==================================================================

int32_t sys_hour(int * hour) {
	*hour = getHour();
	return 0;
}

int32_t sys_minute(int * minute) {
	*minute = getMinute();
	return 0;
}

int32_t sys_second(int * second) {
	*second = getSecond();
	return 0;
}

// ==================================================================
// Draw system calls
// ==================================================================


int32_t sys_circle(uint32_t hexColor, uint64_t topLeftX, uint64_t topLeftY, uint64_t diameter){
	drawCircle(hexColor, topLeftX, topLeftY, diameter);
	return 0;
}

int32_t sys_rectangle(uint32_t color, uint64_t width_pixels, uint64_t height_pixels, uint64_t initial_pos_x, uint64_t initial_pos_y){
	drawRectangle(color, width_pixels, height_pixels, initial_pos_x, initial_pos_y);
	return 0;
}

int32_t sys_fill_video_memory(uint32_t hexColor) {
	fillVideoMemory(hexColor);
	return 0;
}

// ==================================================================
// Custom exec system call
// ==================================================================

int32_t sys_exec(int32_t (*fnPtr)(void)) {
	clear();

	uint8_t fontSize = getFontSize(); 					// preserve font size
	uint32_t text_color = getTextColor();				// preserve text color
	uint32_t background_color = getBackgroundColor();	// preserve background color
	
	SpecialKeyHandler map[ F12_KEY - ESCAPE_KEY + 1 ] = {0};
	SpecialKeyHandler controlMap[ F12_KEY - ESCAPE_KEY + 1 ] = {0};
	clearKeyFnMapNonKernel(map); // avoid """processes/threads/apps""" registering keys across each other over time. reset the map every time
	clearControlKeyFnMapNonKernel(controlMap);
	
	int32_t aux = fnPtr();

	restoreKeyFnMapNonKernel(map);
	restoreControlKeyFnMapNonKernel(controlMap);
	setFontSize(fontSize);
	setTextColor(text_color);
	setBackgroundColor(background_color);

	clear();
	return aux;
}

// ==================================================================
// Custom keyboard system calls
// ==================================================================

int32_t sys_register_key(uint8_t scancode, SpecialKeyHandler fn){
	registerSpecialKey(scancode, fn, 0);
	return 0;
}

int32_t sys_register_ctrl_key(uint8_t scancode, SpecialKeyHandler fn){
	registerControlKey(scancode, fn, 0);
	return 0;
}

// ==================================================================
// Sleep system calls
// ==================================================================
int32_t sys_sleep_milis(uint32_t milis) {
	sleepTicks( (milis * SECONDS_TO_TICKS) / 1000 );
	return 0;
}

// ==================================================================
// Register snapshot system calls
// ==================================================================
int32_t sys_get_register_snapshot(int64_t * registers) {
	if (register_snapshot_taken == 0) return 0;  

	uint8_t i = 0;
	
	while (i < 18) {
		*(registers++) = register_snapshot[i++];
	}

	return 1;
}

int32_t sys_get_character_without_display(void) {
	return getKeyboardCharacter(0);
}
