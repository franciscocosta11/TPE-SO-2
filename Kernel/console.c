// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "ipc.h"
#include "fonts.h"
#include "keyboard.h"
#include "MemoryManager.h"

// Console FD flags
#define CONSOLE_FD_STDIN 0
#define CONSOLE_FD_STDOUT 1
#define CONSOLE_FD_STDERR 2

int consoleWrite(File *fd, const void *buffer, size_t count);
int consoleRead(File *fd, void *buffer, size_t count);
int consoleClose(File *fd);

static FileOps consoleOps = {
    .read = consoleRead,
    .write = consoleWrite,
    .close = consoleClose};

// Constructors to create console-backed File objects
File *createConsoleIn(void)
{
    File *f = (File *)allocMemory(sizeof(*f));
    if (f == NULL)
        return NULL;
    f->ops = &consoleOps;
    f->privateData = NULL; // could hold mode if needed
    f->flags = 0;          // 0 = stdin by convention if needed later
    f->refcount = 1;
    return f;
}

File *createConsoleOut(void)
{
    File *f = (File *)allocMemory(sizeof(*f));
    if (f == NULL)
        return NULL;
    f->ops = &consoleOps;
    f->privateData = NULL;
    f->flags = 1; // stdout tag (optional)
    f->refcount = 1;
    return f;
}

File *createConsoleErr(void)
{
    File *f = (File *)allocMemory(sizeof(*f));
    if (f == NULL)
        return NULL;
    f->ops = &consoleOps;
    f->privateData = NULL;
    f->flags = 2; // stderr tag (optional)
    f->refcount = 1;
    return f;
}

int consoleWrite(File *fd, const void *buffer, size_t count)
{
    (void)fd; // not used yet
    if (buffer == NULL)
    {
        return -1;
    }
    if (count == 0)
    {
        return 0;
    }

    const char *charBuffer = (const char *)buffer;
    size_t i = 0;

    switch (fd != NULL ? fd->flags : CONSOLE_FD_STDOUT)
    {
    case CONSOLE_FD_STDIN: // inject into input buffer (echo on)
        for (; i < count; i++)
        {
            addCharToBuffer((int8_t)charBuffer[i], 1);
        }
        break;
    case CONSOLE_FD_STDERR:
    { // print with error color
        uint32_t prevText = getTextColor();
        uint32_t prevBg = getBackgroundColor();
        setTextColor(DEFAULT_ERROR_COLOR);
        setBackgroundColor(DEFAULT_BACKGROUND_COLOR);
        for (; i < count; i++)
        {
            putChar(charBuffer[i]);
        }
        setTextColor(prevText);
        setBackgroundColor(prevBg);
        break;
    }
    case CONSOLE_FD_STDOUT:
    default:
        for (; i < count; i++)
        {
            putChar(charBuffer[i]);
        }
        break;
    }

    return (int)i;
}

int consoleRead(File *fd, void *buffer, size_t count)
{
    if (buffer == NULL)
    {
        return -1;
    }
    if (count == 0)
    {
        return 0;
    }
    // Solo permitimos leer desde stdin; para stdout/stderr devolvemos error
    if (fd == NULL || fd->flags != CONSOLE_FD_STDIN)
    {
        return -1;
    }
    int c;
    size_t i = 0;
    for (; i < count && (c = getKeyboardCharacter(AWAIT_RETURN_KEY | SHOW_BUFFER_WHILE_TYPING)) != EOF; i++)
    {
        ((char *)buffer)[i] = (char)c;
    }
    return i;
}

int consoleClose(File *fd)
{
    (void)fd;
    return 0;
}
