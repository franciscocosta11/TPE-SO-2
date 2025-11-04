// IPC core helpers: refcounted File objects
#include "ipc.h"
#include "MemoryManager.h"

void fileRetain(File *file)
{
	if (file == NULL)
		return;
	if (file->refcount < 0)
		file->refcount = 0;
	file->refcount++;
}

void fileRelease(File *file)
{
	if (file == NULL)
		return;
	file->refcount--;
	if (file->refcount <= 0)
	{
		if (file->ops && file->ops->close)
		{
			file->ops->close(file);
		}
		freeMemory(file);
	}
}
