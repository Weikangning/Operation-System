.PHONY : target
target :  commonobj create bootloader ISO Kernel qemu
CFLAGS :=  -fno-builtin -w -ggdb -m32 -gstabs -nostdinc -Os -fno-stack-protector
CC := gcc
LD := ld
LDFLAGS := -m elf_i386 -nostdlib -N -T
CommonLib := ./libs
#create Tools
createISO := ./Tools/createISO.c
OutputPathCreateISO := ./bin/CreateISO/createiso

create : $(createISO)
	$(CC) $(createISO) -o $(OutputPathCreateISO)


#create bootloader
BootLoaderInPath := ./BootLoader/

BootLoaderFile := $(shell ls $(BootLoaderInPath))
BootLoaderFile := $(filter %.c %.S,$(BootLoaderFile))
BootLoaderOutPath := ./bin/BootLoader/
BootLoaderOutFile := $(foreach var,$(BootLoaderFile), $(addsuffix .o,$(addprefix $(BootLoaderOutPath),$(var))))
BLldspath :=  ./BootLoader/bootloader.ld
bianryBL := BL
bootloader : $(foreach var,$(BootLoaderFile), $(addprefix $(BootLoaderInPath),$(var)))
	echo $(BootLoaderOutFile) ; \
	for i in $(BootLoaderFile); do  $(CC) -c $(BootLoaderInPath)$$i $(CFLAGS) -I$(CommonLib)  -o $(BootLoaderOutPath)$$i.o; done ; \
 	$(LD)  $(BootLoaderOutFile) $(LDFLAGS) $(BLldspath)  -o ./bin/BootLoader/bootloader ; \
 	objcopy -S -O binary $(BootLoaderOutPath)bootloader  $(BootLoaderOutPath)$(bianryBL)

#编译libs库文件
CommonLibsPath := ./libs/
CommonLibsSrcFile := $(shell ls $(CommonLibsPath))
CommonLibsSrcFile := $(filter %.c , %.cpp %.S , $(CommonLibsSrcFile))
commonobj : $(addprefix libs/,$(CommonLibsSrcFile))
	gcc -Ilibs/ -fno-builtin -w -ggdb -m32 -gstabs -nostdinc  -fno-stack-protector -Ilibs/  -c libs/string.c -o bin/libs/string.o ;\
	gcc -Ilibs/ -fno-builtin -w -ggdb -m32 -gstabs -nostdinc  -fno-stack-protector -Ilibs/  -c libs/printfmt.c -o bin/libs/printfmt.o ;\
	gcc -Ilibs/ -fno-builtin -w -ggdb -m32 -gstabs -nostdinc  -fno-stack-protector -Ilibs/  -c libs/hash.c -o bin/libs/hash.o ;\
	gcc -Ilibs/ -fno-builtin -w -ggdb -m32 -gstabs -nostdinc  -fno-stack-protector -Ilibs/  -c libs/rand.c -o bin/libs/rand.o



#生成Kernel
KernelGCCFlags := -fno-builtin -w -ggdb -m32 -gstabs -nostdinc  -fno-stack-protector
KernelPath := ./Kernel/
KernelFile := $(shell ls $(KernelPath) )
KernelLib :=  $(KernelFile)
KernelFile := $(foreach var,$(KernelFile) ,$(foreach filename, $(shell ls $(KernelPath)$(var)) , $(addprefix  $(KernelPath)$(var)/,$(filename))))
KernelOutPath := ./bin/Kernel/object/
KernelFileSource := $(filter %.c  %.S , $(KernelFile))
KernelFileHead := $(filter %.h , $(KernelFile))
Kernellds := ./Tools/kernel.ld
Kernel : $(KernelFileSource) $(KernelFileHead)
	echo $(KernelFileSource); \
	for i in $(KernelFileSource) ; \
	do  $(CC)  $(KernelGCCFlags) -I$(CommonLib) $(addprefix -I./Kernel/,$(KernelLib)) \
	-c $$i -o $(KernelOutPath)$${i##*/}.o; done ; \
	$(LD) -m  elf_i386 -nostdlib -T Tools/kernel.ld \
	-o bin/Kernel/kernel.exe \
	 ./bin/Kernel/object/*.o


UCoreName :=ucore
ISOName := UCore.img
#生成ISO
ISO :bootloader Kernel
	$(OutputPathCreateISO) bin/BootLoader/$(bianryBL) bin/$(UCoreName) ; \
	dd if=/dev/zero of=bin/$(ISOName) count=10000 ; \
    dd if=bin/$(UCoreName) of=./bin/$(ISOName) conv=notrunc ; \
    dd if=bin/Kernel/kernel.exe of=./bin/$(ISOName) seek=1 conv=notrunc

.PHONY : qemu
qemu :
	qemu-system-i386 -m 512M -parallel stdio -hda bin/$(ISOName) -serial null


.PHONY : clean
clean:
	rm -r ./bin/BootLoader/*  ./bin/CreateISO/* ./bin/Kernel/object/* ./bin/ucore ./bin/UCore.img
