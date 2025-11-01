
MM_STRATEGY ?= MEMORY_MANAGER_SIMPLE

all: bootloader kernel userland image

bootloader:
	$(MAKE) -C Bootloader all

kernel:
	$(MAKE) -C Kernel MM_STRATEGY=$(MM_STRATEGY) all

userland:
	$(MAKE) -C Userland MM_STRATEGY=$(MM_STRATEGY) all

image: kernel bootloader userland
	$(MAKE) -C Image MM_STRATEGY=$(MM_STRATEGY) all

buddy:
	$(MAKE) MM_STRATEGY=MEMORY_MANAGER_BUDDY all

clean:
	$(MAKE) -C Bootloader clean
	$(MAKE) -C Image clean
	$(MAKE) -C Kernel clean
	$(MAKE) -C Userland clean
	rm -f *.zip

.PHONY: bootloader image collections kernel userland all buddy clean
